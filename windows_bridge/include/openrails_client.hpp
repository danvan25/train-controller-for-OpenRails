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

struct CabControlState {
    std::optional<unsigned> throttle_percentage;
    std::optional<unsigned> train_brake_percentage;
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
    void set_train_brake(unsigned percentage);
    CabControlState get_cab_controls();
    double get_speed_kmh();
    std::optional<NextSignalInfo> get_next_signal();

private:
    void set_control(const char* type_name, unsigned percentage);
    std::string get(const wchar_t* path);

    HINTERNET session_ = nullptr;
    HINTERNET connection_ = nullptr;
};

}  // namespace train_controller::bridge
