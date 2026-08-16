#pragma once

#include "openrails_client.hpp"

namespace train_controller::bridge {

enum class EvmState {
    MaximumSpeed,
    Speed120,
    Speed80,
    Speed40,
    PrepareToStop,
    PassedStopSignal,
    NoSignal,
    Shunting
};

EvmState map_to_evm(const NextSignalInfo& signal);
const char* to_string(EvmState state);

}  // namespace train_controller::bridge
