#pragma once

#include <windows.h>
#include <winhttp.h>

#include <optional>
#include <string>

namespace train_controller::bridge {

enum class SignalAspect {
    Clear2,
    Clear1,
    Approach3,
    Approach2,
    Approach1,
    Restricted,
    StopAndProceed,
    Stop,
    Permission,
    None,
    Unknown
};

struct NextSignalInfo {
    SignalAspect aspect = SignalAspect::Unknown;
    std::optional<int> speed_limit_kmh;
    std::optional<double> distance_km;
};

const char* to_string(SignalAspect aspect);

class OpenRailsClient {
public:
    OpenRailsClient();
    ~OpenRailsClient();

    OpenRailsClient(const OpenRailsClient&) = delete;
    OpenRailsClient& operator=(const OpenRailsClient&) = delete;
    OpenRailsClient(OpenRailsClient&&) = delete;
    OpenRailsClient& operator=(OpenRailsClient&&) = delete;

    void set_throttle(unsigned percentage);
    double get_speed_kmh();
    std::optional<NextSignalInfo> get_next_signal();

private:
    std::string get(const wchar_t* path);

    HINTERNET session_ = nullptr;
    HINTERNET connection_ = nullptr;
};

}  // namespace train_controller::bridge
