#include <cstdint>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/structs/io_bank0.h"
#include "hardware/structs/pads_bank0.h"
#include "hardware/structs/sio.h"

namespace
{
    constexpr std::uint32_t LED_PIN = 25;
    constexpr std::uint32_t LED_MASK = 1u << LED_PIN;
}

int main()
{
    // A GPIO 25 fizikai padjának leválasztása az izolált állapotról.
    // Az OD bit törlésével engedélyezzük a kimeneti meghajtást is.
    pads_bank0_hw->io[LED_PIN] &=
        ~(PADS_BANK0_GPIO0_ISO_BITS | PADS_BANK0_GPIO0_OD_BITS);

    // A GPIO 25-öt az SIO perifériához rendeljük.
    io_bank0_hw->io[LED_PIN].ctrl = GPIO_FUNC_SIO;

    // Meghatározott kezdeti állapot: LED kikapcsolva.
    sio_hw->gpio_out &= ~LED_MASK;

    // Kimenet engedélyezése.
    sio_hw->gpio_oe_set = LED_MASK;

    while (true)
    {
        // A GPIO 25 kimeneti állapotának megfordítása.
        sio_hw->gpio_out ^= LED_MASK;

        sleep_ms(500);
    }
}