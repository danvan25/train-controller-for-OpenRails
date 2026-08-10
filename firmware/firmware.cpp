#include <cstdint>

#include "pico/stdlib.h"

#include "hardware/gpio.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/structs/io_bank0.h"
#include "hardware/structs/pads_bank0.h"
#include "hardware/structs/sio.h"

#include "drivers/sh1106.hpp"
#include "graphics/framebuffer.hpp"
#include "graphics/speed_gauge.hpp"
#include "hal/i2c_master.hpp"

namespace
{

constexpr std::uint32_t LED_PIN = 25;
constexpr std::uint32_t LED_MASK = 1u << LED_PIN;

constexpr std::uint16_t MAXIMUM_SPEED = 160;
constexpr std::uint16_t ANIMATION_STEP = 2;
constexpr std::uint32_t FRAME_DELAY_MS = 35;

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
    using train_controller::graphics::Framebuffer;
    using train_controller::graphics::SpeedGauge;
    using train_controller::graphics::SpeedGaugeConfig;
    using train_controller::hal::I2cConfig;
    using train_controller::hal::I2cController;
    using train_controller::hal::I2cMaster;
    using train_controller::hal::I2cResult;

    initialize_status_led();

    const I2cConfig i2c_config {
        I2cController::I2c0,
        4,
        5,
        400'000,
        150'000'000
    };

    const I2cConfig i2c_1_config {
    I2cController::I2c1,
    6,
    7,
    400'000,
    150'000'000
};

    I2cMaster i2c(i2c_config);
I2cMaster i2c_1(i2c_1_config);

if (i2c.initialize() != I2cResult::Ok)
{
    blink_error(100);
}

if (i2c_1.initialize() != I2cResult::Ok)
{
    blink_error(200);
}

Sh1106 display(i2c, 0x3C);
Sh1106 second_display(i2c_1, 0x3C);

if (display.initialize() != I2cResult::Ok)
{
    blink_error(250);
}

if (second_display.initialize() != I2cResult::Ok)
{
    blink_error(400);
}

if (second_display.draw_test_pattern() != I2cResult::Ok)
{
    blink_error(600);
}

    Framebuffer framebuffer;

    const SpeedGaugeConfig gauge_config {
        MAXIMUM_SPEED,
        20,
        10
    };

    const SpeedGauge speed_gauge(gauge_config);

    const auto render_frame =
        [&](std::uint16_t speed) -> I2cResult
        {
            framebuffer.clear();
            speed_gauge.draw(framebuffer, speed);

            return display.present(
                framebuffer.data(),
                framebuffer.size()
            );
        };

    // Solid LED indicates successful initialization.
    set_status_led(true);

    while (true)
    {
        // Acceleration: 0 → 160 km/h.
        for (std::uint16_t speed = 0;
             speed <= MAXIMUM_SPEED;
             speed += ANIMATION_STEP)
        {
            if (render_frame(speed) != I2cResult::Ok)
            {
                blink_error(500);
            }

            sleep_ms(FRAME_DELAY_MS);
        }

        // Deceleration: 158 → 0 km/h.
        for (std::int32_t speed =
                 MAXIMUM_SPEED - ANIMATION_STEP;
             speed >= 0;
             speed -= ANIMATION_STEP)
        {
            if (render_frame(
                    static_cast<std::uint16_t>(speed)
                ) != I2cResult::Ok)
            {
                blink_error(500);
            }

            sleep_ms(FRAME_DELAY_MS);
        }
    }
}