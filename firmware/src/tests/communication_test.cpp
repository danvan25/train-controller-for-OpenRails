#include <cstdio>
#include <cstdint>

#include "pico/stdlib.h"

int main()
{
    stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    std::uint32_t counter = 0;
    bool led_state = false;

    // Időt adunk a Windowsnak az USB-s soros port felismerésére.
    sleep_ms(2000);

    std::printf("PICO_USB_TEST_STARTED\r\n");
    std::fflush(stdout);

    while (true)
    {
        led_state = !led_state;
        gpio_put(PICO_DEFAULT_LED_PIN, led_state);

        std::printf(
            "PICO_USB_TEST counter=%lu uptime_ms=%llu\r\n",
            static_cast<unsigned long>(counter),
            static_cast<unsigned long long>(
                to_ms_since_boot(get_absolute_time())
            )
        );
        std::fflush(stdout);

        ++counter;
        sleep_ms(1000);
    }
}