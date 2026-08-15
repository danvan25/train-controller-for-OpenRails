#include "tests/potentiometer_test.hpp"

#include <cstdio>

namespace train_controller::tests
{

void print_potentiometer(
    const char* name,
    const train_controller::inputs::LinearPotentiometer& potentiometer
)
{
    const std::uint8_t percentage =
        potentiometer.read_percentage();

    std::printf("%s=%u\n",name,static_cast<unsigned>(percentage));
}

}