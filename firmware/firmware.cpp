#include <cstdint>
#include <array>
#include <cstdio>
#include <optional>
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
#include "graphics/evm_display.hpp"
#include "inputs/linear_potentiometer.hpp"
#include "tests/potentiometer_test.hpp"

namespace
{

constexpr std::uint32_t LED_PIN = 25;
constexpr std::uint32_t LED_MASK = 1u << LED_PIN;

constexpr std::uint16_t MAXIMUM_SPEED = 160;

constexpr std::uint32_t POTENTIOMETER_PRINT_INTERVAL_MS = 200;
constexpr std::uint8_t THROTTLE_DEADBAND_PERCENT = 2;
constexpr std::uint32_t MAIN_LOOP_DELAY_MS = 5;

bool line_equals(
    const std::array<char, 32>& line,
    std::size_t length,
    const char* expected
)
{
    std::size_t index = 0;

    while (expected[index] != '\0')
    {
        if (index >= length || line[index] != expected[index])
        {
            return false;
        }

        ++index;
    }

    return index == length;
}

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
    stdio_init_all();

    train_controller::inputs::LinearPotentiometer throttle(26, 0);
    throttle.initialize();

    using train_controller::drivers::Sh1106;

    using train_controller::graphics::EvmDisplay;
    using train_controller::graphics::EvmSignal;
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

/*if (second_display.draw_test_pattern() != I2cResult::Ok)
{
    blink_error(600);
}*/

    Framebuffer speed_framebuffer;
Framebuffer evm_framebuffer;

const SpeedGaugeConfig gauge_config {
    MAXIMUM_SPEED,
    20,
    10
};

const SpeedGauge speed_gauge(gauge_config);

const train_controller::graphics::EvmDisplay evm_display;

EvmSignal displayed_evm_signal = EvmSignal::NoSignal;
bool evm_display_dirty = true;

const auto render_evm_signal =
    [&]() -> I2cResult
    {
        evm_display.draw(
            evm_framebuffer,
            displayed_evm_signal
        );

        return second_display.present(
            evm_framebuffer.data(),
            evm_framebuffer.size()
        );
    };

if (render_evm_signal() != I2cResult::Ok)
{
    blink_error(600);
}

const auto render_speed =
    [&](std::uint16_t speed) -> I2cResult
    {
        speed_framebuffer.clear();

        speed_gauge.draw(
            speed_framebuffer,
            speed
        );

        return display.present(
            speed_framebuffer.data(),
            speed_framebuffer.size()
        );
    };

    // Solid LED indicates successful initialization.
    set_status_led(true);

    absolute_time_t next_potentiometer_print = get_absolute_time();
    std::optional<std::uint8_t> last_sent_throttle;

    const auto run_potentiometer_test_if_due = [&]()
    {
        if (absolute_time_diff_us(
                get_absolute_time(),
                next_potentiometer_print
            ) <= 0)
        {
            const std::uint8_t current_throttle =
                throttle.read_percentage();

            const unsigned difference =
                last_sent_throttle.has_value()
                    ? (current_throttle > *last_sent_throttle
                        ? current_throttle - *last_sent_throttle
                        : *last_sent_throttle - current_throttle)
                    : THROTTLE_DEADBAND_PERCENT;

            if (!last_sent_throttle.has_value() ||
                difference >= THROTTLE_DEADBAND_PERCENT)
            {
                std::printf(
                    "THROTTLE=%u\n",
                    static_cast<unsigned>(current_throttle)
                );

                last_sent_throttle = current_throttle;
            }

            next_potentiometer_print = make_timeout_time_ms(
                POTENTIOMETER_PRINT_INTERVAL_MS
            );
        }
    };

    std::array<char, 32> serial_line {};
    std::size_t serial_line_length = 0;
    std::uint16_t displayed_speed = 0;
    bool speed_display_dirty = true;

    const auto process_serial_input = [&]()
    {
        while (true)
        {
            const int character = getchar_timeout_us(0);

            if (character == PICO_ERROR_TIMEOUT)
            {
                return;
            }

            if (character == '\r')
            {
                continue;
            }

            if (character != '\n')
            {
                if (serial_line_length < serial_line.size())
                {
                    serial_line[serial_line_length++] =
                        static_cast<char>(character);
                }
                else
                {
                    // Discard an overlong or malformed line.
                    serial_line_length = 0;
                }

                continue;
            }

            constexpr std::array<char, 6> SPEED_PREFIX {
                'S', 'P', 'E', 'E', 'D', '='
            };

            bool valid_speed_message =
                serial_line_length > SPEED_PREFIX.size();

            for (std::size_t index = 0;
                 valid_speed_message && index < SPEED_PREFIX.size();
                 ++index)
            {
                valid_speed_message =
                    serial_line[index] == SPEED_PREFIX[index];
            }

            std::uint32_t received_speed = 0;

            for (std::size_t index = SPEED_PREFIX.size();
                 valid_speed_message && index < serial_line_length;
                 ++index)
            {
                const char digit = serial_line[index];

                if (digit < '0' || digit > '9')
                {
                    valid_speed_message = false;
                    break;
                }

                received_speed =
                    received_speed * 10u +
                    static_cast<std::uint32_t>(digit - '0');
            }

            if (valid_speed_message)
            {
                const std::uint16_t limited_speed =
                    received_speed > MAXIMUM_SPEED
                        ? MAXIMUM_SPEED
                        : static_cast<std::uint16_t>(received_speed);

                if (limited_speed != displayed_speed)
                {
                    displayed_speed = limited_speed;
                    speed_display_dirty = true;
                }

                std::printf(
                    "RECEIVED_SPEED=%lu\n",
                    static_cast<unsigned long>(received_speed)
                );
            }

            EvmSignal received_evm_signal = EvmSignal::NoSignal;
            bool valid_evm_message = true;

            if (line_equals(serial_line, serial_line_length,
                            "EVM=MAXIMUM_SPEED"))
            {
                received_evm_signal = EvmSignal::MaximumSpeed;
            }
            else if (line_equals(serial_line, serial_line_length,
                                 "EVM=SPEED_120"))
            {
                received_evm_signal = EvmSignal::Speed120;
            }
            else if (line_equals(serial_line, serial_line_length,
                                 "EVM=SPEED_80"))
            {
                received_evm_signal = EvmSignal::Speed80;
            }
            else if (line_equals(serial_line, serial_line_length,
                                 "EVM=SPEED_40"))
            {
                received_evm_signal = EvmSignal::Speed40;
            }
            else if (line_equals(serial_line, serial_line_length,
                                 "EVM=PREPARE_TO_STOP"))
            {
                received_evm_signal = EvmSignal::PrepareToStop;
            }
            else if (line_equals(serial_line, serial_line_length,
                                 "EVM=PASSED_STOP_SIGNAL"))
            {
                received_evm_signal = EvmSignal::PassedStopSignal;
            }
            else if (line_equals(serial_line, serial_line_length,
                                 "EVM=NO_SIGNAL"))
            {
                received_evm_signal = EvmSignal::NoSignal;
            }
            else if (line_equals(serial_line, serial_line_length,
                                 "EVM=SHUNTING"))
            {
                received_evm_signal = EvmSignal::Shunting;
            }
            else
            {
                valid_evm_message = false;
            }

            if (valid_evm_message &&
                received_evm_signal != displayed_evm_signal)
            {
                displayed_evm_signal = received_evm_signal;
                evm_display_dirty = true;
                std::printf("RECEIVED_EVM=OK\n");
            }

            serial_line_length = 0;
        }
    };

    while (true)
    {
        process_serial_input();
        run_potentiometer_test_if_due();

        if (speed_display_dirty)
        {
            if (render_speed(displayed_speed) != I2cResult::Ok)
            {
                blink_error(500);
            }

            speed_display_dirty = false;
        }

        if (evm_display_dirty)
        {
            if (render_evm_signal() != I2cResult::Ok)
            {
                blink_error(600);
            }

            evm_display_dirty = false;
        }

        sleep_ms(MAIN_LOOP_DELAY_MS);
    }
}
