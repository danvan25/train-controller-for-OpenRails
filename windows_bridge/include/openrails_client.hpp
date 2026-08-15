#pragma once

#include <windows.h>
#include <winhttp.h>

namespace train_controller::bridge {

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

private:
    HINTERNET session_ = nullptr;
    HINTERNET connection_ = nullptr;
};

}  // namespace train_controller::bridge