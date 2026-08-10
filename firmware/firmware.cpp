#include <cstddef>
#include <cstdint>

#include "pico/stdlib.h"

#include "hardware/gpio.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/structs/io_bank0.h"
#include "hardware/structs/pads_bank0.h"
#include "hardware/structs/sio.h"

#include "hal/i2c_master.hpp"

namespace
{

constexpr std::uint32_t LED_PIN = 25;
constexpr std::uint32_t LED_MASK = 1u << LED_PIN;

constexpr std::uint8_t OLED_ADDRESS = 0x3C;

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
    using train_controller::hal::I2cConfig;
    using train_controller::hal::I2cController;
    using train_controller::hal::I2cMaster;
    using train_controller::hal::I2cResult;

    initialize_status_led();

    const I2cConfig config {
        I2cController::I2c0,
        4,
        5,
        100'000,
        150'000'000
    };

    I2cMaster i2c(config);

    const I2cResult initialization_result =
        i2c.initialize();

    if (initialization_result != I2cResult::Ok)
    {
        // Fast blinking: local I2C configuration error.
        blink_error(100);
    }

    // SH1106 control byte:
    // 0x00 means that the following byte is a command.
    //
    // SH1106 command:
    // 0xAE means Display OFF.
    const std::uint8_t display_off_command[] {
        0x00,
        0xAE
    };

    const I2cResult write_result =
        i2c.write(
            OLED_ADDRESS,
            display_off_command,
            sizeof(display_off_command)
        );

    if (write_result == I2cResult::AddressNotAcknowledged)
    {
        // Slow blinking: no device responded at address 0x3C.
        blink_error(500);
    }

    if (write_result != I2cResult::Ok)
    {
        // Medium blinking: another I2C transmission error.
        blink_error(250);
    }

    // Solid LED: the OLED acknowledged the address and command.
    set_status_led(true);

    while (true)
    {
        sleep_ms(1'000);
    }
}