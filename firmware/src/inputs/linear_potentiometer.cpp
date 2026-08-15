#include "inputs/linear_potentiometer.hpp"

#include "hardware/adc.h"

namespace train_controller::inputs
{

LinearPotentiometer::LinearPotentiometer(
    std::uint32_t gpio_pin,
    std::uint32_t adc_channel
)
    : gpio_pin_(gpio_pin),
      adc_channel_(adc_channel)
{
}

void LinearPotentiometer::initialize() const
{
    adc_init();
    adc_gpio_init(gpio_pin_);
}

std::uint16_t LinearPotentiometer::read_raw() const
{
    adc_select_input(adc_channel_);

    return adc_read();
}

std::uint8_t LinearPotentiometer::read_percentage() const
{
    const std::uint32_t raw_value = read_raw();

    return static_cast<std::uint8_t>(
        (raw_value * 100u + ADC_MAXIMUM / 2u)
        / ADC_MAXIMUM
    );
}

}