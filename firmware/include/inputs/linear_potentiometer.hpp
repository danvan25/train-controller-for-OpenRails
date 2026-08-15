#pragma once

#include <cstdint>

namespace train_controller::inputs
{

class LinearPotentiometer
{
public:
    LinearPotentiometer(
        std::uint32_t gpio_pin,
        std::uint32_t adc_channel
    );

    void initialize() const;

    [[nodiscard]] std::uint16_t read_raw() const;
    [[nodiscard]] std::uint8_t read_percentage() const;

private:
    static constexpr std::uint32_t ADC_MAXIMUM = 4095;

    std::uint32_t gpio_pin_;
    std::uint32_t adc_channel_;
};

}