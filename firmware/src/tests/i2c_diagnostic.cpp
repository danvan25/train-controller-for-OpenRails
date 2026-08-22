#include <cstdio>
#include <cstdint>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

namespace
{

constexpr std::uint8_t OLED_ADDRESS = 0x3C;

void initialize_bus(
    i2c_inst_t* bus,
    std::uint32_t sda_pin,
    std::uint32_t scl_pin)
{
    i2c_init(bus, 100'000);

    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);

    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

int test_oled(i2c_inst_t* bus)
{
    // 0xAE: kijelző kikapcsolása
    // 0xA5: minden képpont bekapcsolása
    // 0xAF: kijelző bekapcsolása
    const std::uint8_t commands[] {
        0x00,
        0xAE,
        0xA5,
        0xAF
    };

    return i2c_write_timeout_us(
        bus,
        OLED_ADDRESS,
        commands,
        sizeof(commands),
        false,
        10'000
    );
}

}

int main()
{
    stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, true);

    initialize_bus(i2c0, 0, 1);
    initialize_bus(i2c1, 6, 7);

    sleep_ms(2000);

    std::printf("I2C_DIAGNOSTIC_STARTED\r\n");
    std::fflush(stdout);

    while (true)
    {
        const int first_result = test_oled(i2c0);
        const int second_result = test_oled(i2c1);

        std::printf(
            "I2C0 GP0/GP1: result=%d, SDA=%d, SCL=%d\r\n",
            first_result,
            gpio_get(0) ? 1 : 0,
            gpio_get(1) ? 1 : 0
        );

        std::printf(
            "I2C1 GP6/GP7: result=%d, SDA=%d, SCL=%d\r\n",
            second_result,
            gpio_get(6) ? 1 : 0,
            gpio_get(7) ? 1 : 0
        );

        std::fflush(stdout);
        sleep_ms(2000);
    }
}