#include "evm_mapper.hpp"

namespace train_controller::bridge {
namespace {

EvmState map_speed_limit(int speed_limit_kmh) {
    if (speed_limit_kmh > 120) {
        return EvmState::MaximumSpeed;
    }

    if (speed_limit_kmh > 80) {
        return EvmState::Speed120;
    }

    if (speed_limit_kmh > 40) {
        return EvmState::Speed80;
    }

    return EvmState::Speed40;
}

}  // namespace

EvmState map_to_evm(const NextSignalInfo& signal) {
    switch (signal.aspect) {
        case SignalAspect::Stop:
        case SignalAspect::StopAndProceed:
            return EvmState::PrepareToStop;

        case SignalAspect::None:
        case SignalAspect::Unknown:
            return EvmState::NoSignal;

        case SignalAspect::Restricted:
        case SignalAspect::Permission:
            if (signal.speed_limit_kmh.has_value()) {
                return map_speed_limit(*signal.speed_limit_kmh);
            }

            return EvmState::Speed40;

        case SignalAspect::Approach1:
        case SignalAspect::Approach2:
        case SignalAspect::Approach3:
            if (signal.speed_limit_kmh.has_value()) {
                return map_speed_limit(*signal.speed_limit_kmh);
            }

            return EvmState::PrepareToStop;

        case SignalAspect::Clear1:
        case SignalAspect::Clear2:
            if (signal.speed_limit_kmh.has_value()) {
                return map_speed_limit(*signal.speed_limit_kmh);
            }

            return EvmState::MaximumSpeed;
    }

    return EvmState::NoSignal;
}

const char* to_string(EvmState state) {
    switch (state) {
        case EvmState::MaximumSpeed:     return "MAXIMUM_SPEED";
        case EvmState::Speed120:         return "SPEED_120";
        case EvmState::Speed80:          return "SPEED_80";
        case EvmState::Speed40:          return "SPEED_40";
        case EvmState::PrepareToStop:    return "PREPARE_TO_STOP";
        case EvmState::PassedStopSignal: return "PASSED_STOP_SIGNAL";
        case EvmState::NoSignal:         return "NO_SIGNAL";
        case EvmState::Shunting:         return "SHUNTING";
    }

    return "NO_SIGNAL";
}

}  // namespace train_controller::bridge
