#include "openrails_client.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

std::runtime_error windows_error(const std::string& message) {
    return std::runtime_error(
        message +
        " Windows error code: " +
        std::to_string(GetLastError())
    );
}

std::optional<std::string> json_string(
    std::string_view object,
    std::string_view field
) {
    const std::string key = "\"" + std::string(field) + "\"";
    const std::size_t key_position = object.find(key);

    if (key_position == std::string_view::npos) {
        return std::nullopt;
    }

    const std::size_t colon = object.find(':', key_position + key.size());
    const std::size_t opening_quote = object.find('"', colon + 1);
    const std::size_t closing_quote = object.find('"', opening_quote + 1);

    if (colon == std::string_view::npos ||
        opening_quote == std::string_view::npos ||
        closing_quote == std::string_view::npos) {
        return std::nullopt;
    }

    return std::string(object.substr(
        opening_quote + 1,
        closing_quote - opening_quote - 1
    ));
}

std::optional<int> json_integer(
    std::string_view object,
    std::string_view field
) {
    const std::string key = "\"" + std::string(field) + "\"";
    const std::size_t key_position = object.find(key);

    if (key_position == std::string_view::npos) {
        return std::nullopt;
    }

    const std::size_t colon = object.find(':', key_position + key.size());

    if (colon == std::string_view::npos) {
        return std::nullopt;
    }

    std::size_t number_begin = colon + 1;

    while (number_begin < object.size() &&
           std::isspace(static_cast<unsigned char>(object[number_begin]))) {
        ++number_begin;
    }

    std::size_t number_end = number_begin;

    if (number_end < object.size() && object[number_end] == '-') {
        ++number_end;
    }

    while (number_end < object.size() &&
           std::isdigit(static_cast<unsigned char>(object[number_end]))) {
        ++number_end;
    }

    if (number_end == number_begin) {
        return std::nullopt;
    }

    return std::stoi(std::string(
        object.substr(number_begin, number_end - number_begin)
    ));
}

std::optional<double> json_number(
    std::string_view object,
    std::string_view field
) {
    const std::string key = "\"" + std::string(field) + "\"";
    const std::size_t key_position = object.find(key);

    if (key_position == std::string_view::npos) {
        return std::nullopt;
    }

    const std::size_t colon = object.find(':', key_position + key.size());

    if (colon == std::string_view::npos) {
        return std::nullopt;
    }

    std::size_t number_begin = colon + 1;

    while (number_begin < object.size() &&
           std::isspace(static_cast<unsigned char>(object[number_begin]))) {
        ++number_begin;
    }

    std::size_t number_end = number_begin;

    if (number_end < object.size() && object[number_end] == '-') {
        ++number_end;
    }

    while (number_end < object.size() &&
           (std::isdigit(static_cast<unsigned char>(object[number_end])) ||
            object[number_end] == '.')) {
        ++number_end;
    }

    if (number_end == number_begin) {
        return std::nullopt;
    }

    try {
        return std::stod(std::string(
            object.substr(number_begin, number_end - number_begin)
        ));
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<double> first_number(std::string text) {
    const std::size_t number_begin =
        text.find_first_of("-0123456789");

    if (number_begin == std::string::npos) {
        return std::nullopt;
    }

    const std::size_t number_end =
        text.find_first_not_of("-0123456789,.", number_begin);

    std::string number = text.substr(
        number_begin,
        number_end == std::string::npos
            ? std::string::npos
            : number_end - number_begin
    );

    std::replace(number.begin(), number.end(), ',', '.');

    try {
        return std::stod(number);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

train_controller::bridge::SignalAspect signal_aspect_from_sprite(
    int x,
    int y
) {
    using train_controller::bridge::SignalAspect;

    if (x == 0 && y == 0)   return SignalAspect::Clear2;
    if (x == 16 && y == 0)  return SignalAspect::Clear1;
    if (x == 0 && y == 16)  return SignalAspect::Approach3;
    if (x == 16 && y == 16) return SignalAspect::Approach2;
    if (x == 0 && y == 32)  return SignalAspect::Approach1;
    if (x == 16 && y == 32) return SignalAspect::Restricted;
    if (x == 0 && y == 48)  return SignalAspect::StopAndProceed;
    if (x == 16 && y == 48) return SignalAspect::Stop;
    if (x == 0 && y == 64)  return SignalAspect::Permission;
    if (x == 16 && y == 64) return SignalAspect::None;

    return SignalAspect::Unknown;
}

}  // namespace

namespace train_controller::bridge {

const char* to_string(SignalAspect aspect) {
    switch (aspect) {
        case SignalAspect::Clear2:         return "CLEAR_2";
        case SignalAspect::Clear1:         return "CLEAR_1";
        case SignalAspect::Approach3:      return "APPROACH_3";
        case SignalAspect::Approach2:      return "APPROACH_2";
        case SignalAspect::Approach1:      return "APPROACH_1";
        case SignalAspect::Restricted:     return "RESTRICTED";
        case SignalAspect::StopAndProceed: return "STOP_AND_PROCEED";
        case SignalAspect::Stop:           return "STOP";
        case SignalAspect::Permission:     return "PERMISSION";
        case SignalAspect::None:           return "NONE";
        default:                           return "UNKNOWN";
    }
}

OpenRailsClient::OpenRailsClient() {
    session_ = WinHttpOpen(
        L"OpenRailsBridge/0.1",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (session_ == nullptr) {
        throw windows_error("Could not initialize WinHTTP.");
    }

    connection_ = WinHttpConnect(session_, L"localhost", 2150, 0);

    if (connection_ == nullptr) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(session_);
        session_ = nullptr;
        SetLastError(error_code);

        throw windows_error("Could not connect to Open Rails.");
    }
}

OpenRailsClient::~OpenRailsClient() {
    if (connection_ != nullptr) {
        WinHttpCloseHandle(connection_);
    }

    if (session_ != nullptr) {
        WinHttpCloseHandle(session_);
    }
}

void OpenRailsClient::set_throttle(unsigned percentage) {
    set_control("THROTTLE", percentage);
}

void OpenRailsClient::set_train_brake(unsigned percentage) {
    set_control("TRAIN_BRAKE", percentage);
}

void OpenRailsClient::set_control(
    const char* type_name,
    unsigned percentage
) {
    const double normalized_value =
        static_cast<double>(percentage) / 100.0;

    std::ostringstream json_stream;
    json_stream
        << std::fixed
        << std::setprecision(2)
        << R"([{"TypeName":")"
        << type_name
        << R"(","Value":)"
        << normalized_value
        << R"(}])";

    const std::string request_body = json_stream.str();

    HINTERNET request = WinHttpOpenRequest(
        connection_,
        L"POST",
        L"/API/CABCONTROLS",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );

    if (request == nullptr) {
        throw windows_error("Could not create HTTP request.");
    }

    const wchar_t* headers = L"Content-Type: application/json\r\n";

    const BOOL sent = WinHttpSendRequest(
        request,
        headers,
        static_cast<DWORD>(-1L),
        const_cast<char*>(request_body.data()),
        static_cast<DWORD>(request_body.size()),
        static_cast<DWORD>(request_body.size()),
        0
    );

    if (!sent) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(request);
        SetLastError(error_code);
        throw windows_error("Could not send HTTP request.");
    }

    if (!WinHttpReceiveResponse(request, nullptr)) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(request);
        SetLastError(error_code);
        throw windows_error("No response received from Open Rails.");
    }

    DWORD status_code = 0;
    DWORD status_code_size = sizeof(status_code);

    const BOOL status_received = WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &status_code,
        &status_code_size,
        WINHTTP_NO_HEADER_INDEX
    );

    if (!status_received) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(request);
        SetLastError(error_code);
        throw windows_error("Could not read HTTP status code.");
    }

    WinHttpCloseHandle(request);

    if (status_code < 200 || status_code >= 300) {
        throw std::runtime_error(
            "Open Rails returned HTTP status " +
            std::to_string(status_code)
        );
    }
}

std::string OpenRailsClient::get(const wchar_t* path) {
    HINTERNET request = WinHttpOpenRequest(
        connection_,
        L"GET",
        path,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );

    if (request == nullptr) {
        throw windows_error("Could not create GET request.");
    }

    if (!WinHttpSendRequest(
            request,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0)) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(request);
        SetLastError(error_code);
        throw windows_error("Could not send GET request.");
    }

    if (!WinHttpReceiveResponse(request, nullptr)) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(request);
        SetLastError(error_code);
        throw windows_error("No GET response received from Open Rails.");
    }

    DWORD status_code = 0;
    DWORD status_code_size = sizeof(status_code);

    if (!WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status_code,
            &status_code_size,
            WINHTTP_NO_HEADER_INDEX)) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(request);
        SetLastError(error_code);
        throw windows_error("Could not read GET response status.");
    }

    if (status_code < 200 || status_code >= 300) {
        WinHttpCloseHandle(request);
        throw std::runtime_error(
            "Open Rails returned HTTP status " +
            std::to_string(status_code)
        );
    }

    std::string response_body;

    while (true) {
        DWORD available_bytes = 0;

        if (!WinHttpQueryDataAvailable(request, &available_bytes)) {
            const DWORD error_code = GetLastError();
            WinHttpCloseHandle(request);
            SetLastError(error_code);
            throw windows_error("Could not query GET response data.");
        }

        if (available_bytes == 0) {
            break;
        }

        const std::size_t old_size = response_body.size();
        response_body.resize(old_size + available_bytes);

        DWORD bytes_read = 0;

        if (!WinHttpReadData(
                request,
                response_body.data() + old_size,
                available_bytes,
                &bytes_read)) {
            const DWORD error_code = GetLastError();
            WinHttpCloseHandle(request);
            SetLastError(error_code);
            throw windows_error("Could not read GET response data.");
        }

        response_body.resize(old_size + bytes_read);
    }

    WinHttpCloseHandle(request);
    return response_body;
}

double OpenRailsClient::get_speed_kmh() {
    const std::string response_body =
        get(L"/API/TRAINDRIVINGDISPLAY");

    const std::size_t speed_key = response_body.find(R"("SPED")");

    if (speed_key == std::string::npos) {
        throw std::runtime_error(
            "SPED field not found in Open Rails response."
        );
    }

    const std::size_t row_end = response_body.find("},", speed_key);
    const std::string_view speed_row(
        response_body.data() + speed_key,
        (row_end == std::string::npos ? response_body.size() : row_end) -
            speed_key
    );

    const auto speed_text = json_string(speed_row, "LastCol");

    if (!speed_text.has_value()) {
        throw std::runtime_error("LastCol field not found after SPED.");
    }

    const auto speed = first_number(*speed_text);

    if (!speed.has_value()) {
        throw std::runtime_error("Invalid Open Rails speed value.");
    }

    return *speed;
}

CabControlState OpenRailsClient::get_cab_controls() {
    const std::string response_body = get(L"/API/CABCONTROLS");
    std::size_t object_begin = 0;
    CabControlState state;

    while (true) {
        object_begin = response_body.find('{', object_begin);

        if (object_begin == std::string::npos) {
            return state;
        }

        const std::size_t object_end =
            response_body.find('}', object_begin + 1);

        if (object_end == std::string::npos) {
            return state;
        }

        const std::string_view object(
            response_body.data() + object_begin,
            object_end - object_begin + 1
        );

        const auto type_name = json_string(object, "TypeName");

        if (type_name.has_value() &&
            (*type_name == "THROTTLE" ||
             *type_name == "TRAIN_BRAKE")) {
            const auto fraction = json_number(object, "RangeFraction");

            if (fraction.has_value()) {
                const double limited_fraction =
                    std::clamp(*fraction, 0.0, 1.0);

                const unsigned percentage =
                    static_cast<unsigned>(
                        limited_fraction * 100.0 + 0.5
                    );

                if (*type_name == "THROTTLE") {
                    state.throttle_percentage = percentage;
                } else {
                    state.train_brake_percentage = percentage;
                }
            }
        }

        object_begin = object_end + 1;
    }
}

std::optional<NextSignalInfo> OpenRailsClient::get_next_signal() {
    const std::string response_body =
        get(L"/API/TRACKMONITORDISPLAY");

    std::size_t row_begin = 0;
    std::optional<NextSignalInfo> nearest_forward_signal;

    while (true) {
        row_begin = response_body.find(R"("FirstCol")", row_begin);

        if (row_begin == std::string::npos) {
            return std::nullopt;
        }

        const std::size_t next_row =
            response_body.find(R"("FirstCol")", row_begin + 1);

        const std::size_t row_end =
            next_row == std::string::npos
                ? response_body.size()
                : next_row;

        const std::string_view row(
            response_body.data() + row_begin,
            row_end - row_begin
        );

        const std::size_t track_item =
            row.find(R"("TrackColItem")");

        if (track_item != std::string_view::npos) {
            const std::string_view sprite = row.substr(track_item);

            const auto x = json_integer(sprite, "X");
            const auto y = json_integer(sprite, "Y");
            const auto width = json_integer(sprite, "Width");
            const auto height = json_integer(sprite, "Height");

            const bool is_train_marker =
                x.has_value() && y.has_value() &&
                width == 24 && height == 24 &&
                ((*x == 0 && *y == 72) ||
                 (*x == 24 && *y == 72) ||
                 (*x == 24 && *y == 96) ||
                 (*x == 0 && *y == 96));

            if (is_train_marker) {
                return nearest_forward_signal;
            }
        }

        const std::size_t signal_item =
            row.find(R"("SignalColItem")");

        if (signal_item != std::string_view::npos) {
            const std::string_view sprite = row.substr(signal_item);

            const auto x = json_integer(sprite, "X");
            const auto y = json_integer(sprite, "Y");
            const auto width = json_integer(sprite, "Width");
            const auto height = json_integer(sprite, "Height");

            if (x.has_value() && y.has_value() &&
                width == 16 && height == 16) {
                NextSignalInfo info;
                info.aspect = signal_aspect_from_sprite(*x, *y);

                if (const auto limit = json_string(row, "LimitCol")) {
                    if (const auto value = first_number(*limit)) {
                        info.speed_limit_kmh =
                            static_cast<int>(*value + 0.5);
                    }
                }

                if (const auto distance = json_string(row, "DistCol")) {
                    if (auto value = first_number(*distance)) {
                        if (distance->find("km") == std::string::npos &&
                            distance->find('m') != std::string::npos) {
                            *value /= 1000.0;
                        }

                        info.distance_km = *value;
                    }
                }

                // Forward items are ordered from the farthest item toward
                // the train marker, so each match replaces the previous one.
                nearest_forward_signal = info;
            }
        }

        if (next_row == std::string::npos) {
            return nearest_forward_signal;
        }

        row_begin = next_row;
    }
}

}  // namespace train_controller::bridge
