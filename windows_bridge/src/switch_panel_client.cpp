#include "switch_panel_client.hpp"

#include <array>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

std::runtime_error websocket_error(const std::string& message) {
    return std::runtime_error(
        message + " Windows error code: " +
        std::to_string(GetLastError())
    );
}

std::optional<bool> decode_state(int value) {
    if (value == 0) return false;
    if (value == 1) return true;
    return std::nullopt;
}

}  // namespace

namespace train_controller::bridge {

SwitchPanelClient::SwitchPanelClient() {
    session_ = WinHttpOpen(
        L"OpenRailsBridge/0.2",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (session_ == nullptr) {
        throw websocket_error("Could not initialize Switch Panel session.");
    }

    connection_ = WinHttpConnect(session_, L"localhost", 2150, 0);
    if (connection_ == nullptr) {
        throw websocket_error("Could not connect to Switch Panel.");
    }

    HINTERNET request = WinHttpOpenRequest(
        connection_, L"GET", L"/switchpanel", nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0
    );
    if (request == nullptr) {
        throw websocket_error("Could not create Switch Panel request.");
    }

    if (!WinHttpSetOption(
            request,
            WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
            nullptr,
            0)) {
        WinHttpCloseHandle(request);
        throw websocket_error("Could not request WebSocket upgrade.");
    }

    const wchar_t* headers = L"Sec-WebSocket-Protocol: json\r\n";
    if (!WinHttpSendRequest(
            request, headers, static_cast<DWORD>(-1L),
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request);
        throw websocket_error("Switch Panel handshake failed.");
    }

    websocket_ = WinHttpWebSocketCompleteUpgrade(request, 0);
    WinHttpCloseHandle(request);

    if (websocket_ == nullptr) {
        throw websocket_error("Could not complete WebSocket upgrade.");
    }

    receiver_thread_ = std::thread(&SwitchPanelClient::receive_loop, this);
}

SwitchPanelClient::~SwitchPanelClient() {
    stopping_ = true;

    if (websocket_ != nullptr) {
        WinHttpWebSocketClose(
            websocket_,
            WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS,
            nullptr,
            0
        );
    }

    if (receiver_thread_.joinable()) receiver_thread_.join();
    if (websocket_ != nullptr) WinHttpCloseHandle(websocket_);
    if (connection_ != nullptr) WinHttpCloseHandle(connection_);
    if (session_ != nullptr) WinHttpCloseHandle(session_);
}

void SwitchPanelClient::set_button(int user_command, bool pressed) {
    send_json(
        "{\"type\":\"" +
        std::string(pressed ? "buttonDown" : "buttonUp") +
        "\",\"data\":" + std::to_string(user_command) + "}"
    );
}

void SwitchPanelClient::pulse_button(int user_command) {
    set_button(user_command, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    set_button(user_command, false);
}

std::optional<bool> SwitchPanelClient::pantograph_1_up() const {
    return decode_state(pantograph_1_state_);
}

std::optional<bool> SwitchPanelClient::pantograph_2_up() const {
    return decode_state(pantograph_2_state_);
}

void SwitchPanelClient::send_json(const std::string& json) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    const DWORD result = WinHttpWebSocketSend(
        websocket_,
        WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        const_cast<char*>(json.data()),
        static_cast<DWORD>(json.size())
    );
    if (result != ERROR_SUCCESS) {
        SetLastError(result);
        throw websocket_error("Could not send Switch Panel command.");
    }
}

void SwitchPanelClient::receive_loop() {
    std::array<char, 8192> buffer {};
    std::string message;

    while (!stopping_) {
        DWORD bytes_read = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE type;
        const DWORD result = WinHttpWebSocketReceive(
            websocket_, buffer.data(), static_cast<DWORD>(buffer.size()),
            &bytes_read, &type
        );

        if (result != ERROR_SUCCESS ||
            type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
            return;
        }

        message.append(buffer.data(), bytes_read);
        if (type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) {
            process_message(message);
            message.clear();
        }
    }
}

void SwitchPanelClient::process_message(const std::string& message) {
    update_pantograph_state(message, 153, pantograph_1_state_);
    update_pantograph_state(message, 154, pantograph_2_state_);
}

void SwitchPanelClient::update_pantograph_state(
    const std::string& message,
    int user_command,
    std::atomic<int>& state
) {
    const std::string command =
        "\"UserCommand\":[" + std::to_string(user_command) + "]";
    std::size_t position = 0;

    while ((position = message.find(command, position)) != std::string::npos) {
        const std::size_t next_definition =
            message.find("\"Definition\"", position + command.size());
        const std::size_t status_position =
            message.find("\"Status\":{\"Status\":\"", position);

        if (status_position == std::string::npos ||
            (next_definition != std::string::npos &&
             status_position > next_definition)) {
            position += command.size();
            continue;
        }

        const std::size_t value_begin =
            status_position + std::string("\"Status\":{\"Status\":\"").size();
        const std::size_t value_end = message.find('"', value_begin);
        if (value_end == std::string::npos) return;

        const std::string value =
            message.substr(value_begin, value_end - value_begin);
        if (value == "Fel" || value == "Up") state = 1;
        if (value == "Le" || value == "Down") state = 0;
        return;
    }
}

}  // namespace train_controller::bridge
