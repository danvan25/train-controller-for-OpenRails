#include "control_profile.hpp"

namespace train_controller::bridge {

namespace {

constexpr ControlProfile CONVENTIONAL_LOCOMOTIVE {
    "conventional_locomotive",
    SecondaryBrakeTarget::EngineBrake,
    "ENGINE_BRAKE"
};

}  // namespace

ControlProfile default_control_profile() {
    return CONVENTIONAL_LOCOMOTIVE;
}

std::optional<ControlProfile> find_control_profile(std::string_view name) {
    if (name == CONVENTIONAL_LOCOMOTIVE.name) {
        return CONVENTIONAL_LOCOMOTIVE;
    }

    return std::nullopt;
}

}  // namespace train_controller::bridge
