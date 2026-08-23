#pragma once

#include <windows.h>
#include <winhttp.h>

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace train_controller::bridge {

class SwitchPanelClient {
public:
    SwitchPanelClient();
    ~SwitchPanelClient();

    SwitchPanelClient(const SwitchPanelClient&) = delete;
    SwitchPanelClient& operator=(const SwitchPanelClient&) = delete;

    void set_button(int user_command, bool pressed);
    void pulse_button(int user_command);

    std::optional<bool> pantograph_1_up() const;
    std::optional<bool> pantograph_2_up() const;

private:
    void receive_loop();
    void process_message(const std::string& message);
    void update_pantograph_state(
        const std::string& message,
        int user_command,
        std::atomic<int>& state
    );
    void send_json(const std::string& json);

    HINTERNET session_ = nullptr;
    HINTERNET connection_ = nullptr;
    HINTERNET websocket_ = nullptr;
    std::atomic<bool> stopping_ {false};
    std::atomic<int> pantograph_1_state_ {-1};
    std::atomic<int> pantograph_2_state_ {-1};
    std::mutex send_mutex_;
    std::thread receiver_thread_;
};

}  // namespace train_controller::bridge
