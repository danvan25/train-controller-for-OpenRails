#pragma once

#include <cstddef>
#include <cstdint>

#include "hal/i2c_master.hpp"

namespace train_controller::drivers
{

class Sh1106 final
{
public:
    static constexpr std::uint8_t WIDTH = 128;
    static constexpr std::uint8_t HEIGHT = 64;
    static constexpr std::uint8_t PAGE_COUNT = HEIGHT / 8;

    explicit Sh1106(
        hal::I2cMaster& i2c,
        std::uint8_t address = 0x3C
    );

    [[nodiscard]] hal::I2cResult initialize();
    [[nodiscard]] hal::I2cResult clear();
    [[nodiscard]] hal::I2cResult draw_test_pattern();

private:
    static constexpr std::size_t CONTROLLER_WIDTH = 132;
    static constexpr std::size_t MAX_COMMAND_COUNT = 32;

    [[nodiscard]] hal::I2cResult send_commands(
        const std::uint8_t* commands,
        std::size_t length
    );

    hal::I2cMaster& i2c_;
    std::uint8_t address_;
};

}
