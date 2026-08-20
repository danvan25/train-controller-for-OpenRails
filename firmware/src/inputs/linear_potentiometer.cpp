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

    // The first conversion after switching ADC channels may still contain
    // charge from the previously selected input, so discard it.
    static_cast<void>(adc_read());

    constexpr std::uint32_t SAMPLE_COUNT = 4;
    std::uint32_t sum = 0;

    for (std::uint32_t sample = 0; sample < SAMPLE_COUNT; ++sample)
    {
        sum += adc_read();
    }

    return static_cast<std::uint16_t>(
        (sum + SAMPLE_COUNT / 2u) / SAMPLE_COUNT
    );
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
