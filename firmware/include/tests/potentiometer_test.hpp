#pragma once

#include "inputs/linear_potentiometer.hpp"

namespace train_controller::tests
{

void print_potentiometer(
    const char* name,
    const train_controller::inputs::LinearPotentiometer& potentiometer
);

}