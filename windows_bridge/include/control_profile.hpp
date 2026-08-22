#pragma once

#include <optional>
#include <string_view>

namespace train_controller::bridge {

enum class SecondaryBrakeTarget {
    EngineBrake,
    DynamicBrake,
    Ignored
};

struct ControlProfile {
    std::string_view name;
    SecondaryBrakeTarget secondary_brake_target;
    std::string_view open_rails_control_name;
};

ControlProfile default_control_profile();
std::optional<ControlProfile> find_control_profile(std::string_view name);

}  // namespace train_controller::bridge
