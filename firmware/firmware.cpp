#include <cstdint>

#include "pico/stdlib.h"

#include "hardware/gpio.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/structs/io_bank0.h"
#include "hardware/structs/pads_bank0.h"
#include "hardware/structs/sio.h"

#include "drivers/sh1106.hpp"
#include "hal/i2c_master.hpp"

namespace
{

constexpr std::uint32_t LED_PIN = 25;
constexpr std::uint32_t LED_MASK = 1u << LED_PIN;

void initialize_status_led()
{
    pads_bank0_hw->io[LED_PIN] &=
        ~(PADS_BANK0_GPIO0_ISO_BITS |
          PADS_BANK0_GPIO0_OD_BITS);

    io_bank0_hw->io[LED_PIN].ctrl = GPIO_FUNC_SIO;

    sio_hw->gpio_out &= ~LED_MASK;
    sio_hw->gpio_oe_set = LED_MASK;
}

void set_status_led(bool enabled)
{
    if (enabled)
    {
        sio_hw->gpio_out |= LED_MASK;
    }
    else
    {
        sio_hw->gpio_out &= ~LED_MASK;
    }
}

[[noreturn]] void blink_error(std::uint32_t interval_ms)
{
    while (true)
    {
        sio_hw->gpio_out ^= LED_MASK;
        sleep_ms(interval_ms);
    }
}

}

int main()
{
    using train_controller::drivers::Sh1106;
    using train_controller::hal::I2cConfig;
    using train_controller::hal::I2cController;
    using train_controller::hal::I2cMaster;
    using train_controller::hal::I2cResult;

    initialize_status_led();

    const I2cConfig i2c_config {
        I2cController::I2c0,
        4,
        5,
        100'000,
        150'000'000
    };

    I2cMaster i2c(i2c_config);

    const I2cResult i2c_result =
        i2c.initialize();

    if (i2c_result != I2cResult::Ok)
    {
        // Very fast blinking: I2C configuration failed.
        blink_error(100);
    }

    Sh1106 display(i2c);

    const I2cResult display_result =
        display.initialize();

    if (display_result != I2cResult::Ok)
    {
        // Medium blinking: SH1106 initialization failed.
        blink_error(250);
    }

    const I2cResult pattern_result =
        display.draw_test_pattern();

    if (pattern_result != I2cResult::Ok)
    {
        // Slow blinking: display-data transmission failed.
        blink_error(500);
    }

    // Solid LED means initialization and drawing both succeeded.
    set_status_led(true);

    while (true)
    {
        sleep_ms(1'000);
    }
}