#include "openrails_client.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <algorithm>

namespace {

std::runtime_error windows_error(const std::string& message) {
    return std::runtime_error(
        message +
        " Windows error code: " +
        std::to_string(GetLastError())
    );
}

}  // namespace

namespace train_controller::bridge {

OpenRailsClient::OpenRailsClient() {
    session_ = WinHttpOpen(
        L"OpenRailsBridge/0.1",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (session_ == nullptr) {
        throw windows_error(
            "Could not initialize WinHTTP."
        );
    }

    connection_ = WinHttpConnect(
        session_,
        L"localhost",
        2150,
        0
    );

    if (connection_ == nullptr) {
        WinHttpCloseHandle(session_);
        session_ = nullptr;

        throw windows_error(
            "Could not connect to Open Rails."
        );
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
    const double normalized_value =
        static_cast<double>(percentage) / 100.0;

    std::ostringstream json_stream;

    json_stream
        << std::fixed
        << std::setprecision(2)
        << R"([{"TypeName":"THROTTLE","Value":)"
        << normalized_value
        << R"(}])";

    const std::string request_body =
        json_stream.str();

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
        throw windows_error(
            "Could not create HTTP request."
        );
    }

    const wchar_t* headers =
        L"Content-Type: application/json\r\n";

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

        throw windows_error(
            "Could not send HTTP request."
        );
    }

    if (!WinHttpReceiveResponse(request, nullptr)) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(request);
        SetLastError(error_code);

        throw windows_error(
            "No response received from Open Rails."
        );
    }

    DWORD status_code = 0;
    DWORD status_code_size = sizeof(status_code);

    const BOOL status_received = WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE |
            WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &status_code,
        &status_code_size,
        WINHTTP_NO_HEADER_INDEX
    );

    if (!status_received) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(request);
        SetLastError(error_code);

        throw windows_error(
            "Could not read HTTP status code."
        );
    }

    WinHttpCloseHandle(request);

    if (status_code < 200 || status_code >= 300) {
        throw std::runtime_error(
            "Open Rails returned HTTP status " +
            std::to_string(status_code)
        );
    }
}

double OpenRailsClient::get_speed_kmh() {
    HINTERNET request = WinHttpOpenRequest(
        connection_,
        L"GET",
        L"/API/TRAINDRIVINGDISPLAY",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );

    if (request == nullptr) {
        throw windows_error(
            "Could not create speed request."
        );
    }

    const BOOL sent = WinHttpSendRequest(
        request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );

    if (!sent) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(request);
        SetLastError(error_code);

        throw windows_error(
            "Could not send speed request."
        );
    }

    if (!WinHttpReceiveResponse(request, nullptr)) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(request);
        SetLastError(error_code);

        throw windows_error(
            "No speed response received from Open Rails."
        );
    }

    DWORD status_code = 0;
    DWORD status_code_size = sizeof(status_code);

    const BOOL status_received = WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE |
            WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &status_code,
        &status_code_size,
        WINHTTP_NO_HEADER_INDEX
    );

    if (!status_received) {
        const DWORD error_code = GetLastError();
        WinHttpCloseHandle(request);
        SetLastError(error_code);

        throw windows_error(
            "Could not read speed response status."
        );
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

        if (!WinHttpQueryDataAvailable(
                request,
                &available_bytes)) {

            const DWORD error_code = GetLastError();
            WinHttpCloseHandle(request);
            SetLastError(error_code);

            throw windows_error(
                "Could not query speed response data."
            );
        }

        if (available_bytes == 0) {
            break;
        }

        const std::size_t old_size =
            response_body.size();

        response_body.resize(
            old_size + available_bytes
        );

        DWORD bytes_read = 0;

        if (!WinHttpReadData(
                request,
                response_body.data() + old_size,
                available_bytes,
                &bytes_read)) {

            const DWORD error_code = GetLastError();
            WinHttpCloseHandle(request);
            SetLastError(error_code);

            throw windows_error(
                "Could not read speed response."
            );
        }

        response_body.resize(
            old_size + bytes_read
        );
    }

    WinHttpCloseHandle(request);

    const std::size_t speed_key =
        response_body.find(R"("SPED")");

    if (speed_key == std::string::npos) {
        throw std::runtime_error(
            "SPED field not found in Open Rails response."
        );
    }

    const std::size_t last_col =
        response_body.find(
            R"("LastCol")",
            speed_key
        );

    if (last_col == std::string::npos) {
        throw std::runtime_error(
            "LastCol field not found after SPED."
        );
    }

    const std::size_t colon =
        response_body.find(':', last_col);

    const std::size_t opening_quote =
        response_body.find('"', colon);

    const std::size_t closing_quote =
        response_body.find(
            '"',
            opening_quote + 1
        );

    if (colon == std::string::npos ||
        opening_quote == std::string::npos ||
        closing_quote == std::string::npos) {

        throw std::runtime_error(
            "Invalid SPED field format."
        );
    }

    std::string speed_text =
        response_body.substr(
            opening_quote + 1,
            closing_quote - opening_quote - 1
        );

    const std::size_t numeric_end =
        speed_text.find_first_not_of(
            "0123456789,.-"
        );

    if (numeric_end != std::string::npos) {
        speed_text.resize(numeric_end);
    }

    if (speed_text.empty()) {
        throw std::runtime_error(
            "Open Rails returned an empty speed value."
        );
    }

    std::replace(
        speed_text.begin(),
        speed_text.end(),
        ',',
        '.'
    );

    return std::stod(speed_text);
}

}  // namespace train_controller::bridge