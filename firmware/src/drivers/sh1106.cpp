#include "drivers/sh1106.hpp"

#include <array>

namespace train_controller::drivers
{

namespace
{

constexpr std::uint8_t COMMAND_CONTROL_BYTE = 0x00;
constexpr std::uint8_t DATA_CONTROL_BYTE = 0x40;

constexpr std::uint8_t DISPLAY_OFF = 0xAE;
constexpr std::uint8_t DISPLAY_ON = 0xAF;

}

Sh1106::Sh1106(
    hal::I2cMaster& i2c,
    std::uint8_t address
)
    : i2c_(i2c),
      address_(address)
{
}

hal::I2cResult Sh1106::initialize()
{
    const std::uint8_t initialization_commands[] {
        DISPLAY_OFF,

        0xD5, 0x80,   // Display clock division and oscillator frequency
        0xA8, 0x3F,   // Multiplex ratio: 1/64
        0xD3, 0x00,   // Display offset: 0
        0x40,         // Display start line: 0

        0xAD, 0x8B,   // Enable the internal DC-DC converter

        0xA1,         // Segment remap
        0xC8,         // Reverse COM scan direction

        0xDA, 0x12,   // COM pin hardware configuration
        0x81, 0x7F,   // Contrast
        0xD9, 0x22,   // Pre-charge period
        0xDB, 0x35,   // VCOM deselect level

        0xA4,         // Display follows RAM contents
        0xA6          // Normal, non-inverted display
    };

    hal::I2cResult result =
        send_commands(
            initialization_commands,
            sizeof(initialization_commands)
        );

    if (result != hal::I2cResult::Ok)
    {
        return result;
    }

    result = clear();

    if (result != hal::I2cResult::Ok)
    {
        return result;
    }

    const std::uint8_t display_on_command[] {
        DISPLAY_ON
    };

    return send_commands(
        display_on_command,
        sizeof(display_on_command)
    );
}

hal::I2cResult Sh1106::clear()
{
    std::array<std::uint8_t, CONTROLLER_WIDTH + 1>
        clear_data {};

    clear_data[0] = DATA_CONTROL_BYTE;

    for (std::uint8_t page = 0;
         page < PAGE_COUNT;
         ++page)
    {
        const std::uint8_t page_commands[] {
            static_cast<std::uint8_t>(0xB0 | page),
            0x00,
            0x10
        };

        hal::I2cResult result =
            send_commands(
                page_commands,
                sizeof(page_commands)
            );

        if (result != hal::I2cResult::Ok)
        {
            return result;
        }

        result = i2c_.write(
            address_,
            clear_data.data(),
            clear_data.size()
        );

        if (result != hal::I2cResult::Ok)
        {
            return result;
        }
    }

    return hal::I2cResult::Ok;
}

hal::I2cResult Sh1106::draw_test_pattern()
{
    constexpr std::uint8_t COLUMN_OFFSET = 2;
    constexpr std::uint8_t BLOCK_WIDTH = 8;

    std::array<std::uint8_t, WIDTH + 1>
        display_data {};

    display_data[0] = DATA_CONTROL_BYTE;

    for (std::uint8_t page = 0;
         page < PAGE_COUNT;
         ++page)
    {
        const std::uint8_t page_commands[] {
            static_cast<std::uint8_t>(0xB0 | page),
            static_cast<std::uint8_t>(
                COLUMN_OFFSET & 0x0F
            ),
            static_cast<std::uint8_t>(
                0x10 | (COLUMN_OFFSET >> 4)
            )
        };

        hal::I2cResult result =
            send_commands(
                page_commands,
                sizeof(page_commands)
            );

        if (result != hal::I2cResult::Ok)
        {
            return result;
        }

        for (std::size_t column = 0;
             column < WIDTH;
             ++column)
        {
            const std::size_t horizontal_block =
                column / BLOCK_WIDTH;

            const bool block_is_enabled =
                ((horizontal_block + page) % 2) == 0;

            display_data[column + 1] =
                block_is_enabled ? 0xFF : 0x00;
        }

        result = i2c_.write(
            address_,
            display_data.data(),
            display_data.size()
        );

        if (result != hal::I2cResult::Ok)
        {
            return result;
        }
    }

    return hal::I2cResult::Ok;
}

hal::I2cResult Sh1106::send_commands(
    const std::uint8_t* commands,
    std::size_t length
)
{
    if (commands == nullptr ||
        length == 0 ||
        length > MAX_COMMAND_COUNT)
    {
        return hal::I2cResult::InvalidArgument;
    }

    std::array<std::uint8_t, MAX_COMMAND_COUNT + 1>
        transmission {};

    transmission[0] = COMMAND_CONTROL_BYTE;

    for (std::size_t index = 0;
         index < length;
         ++index)
    {
        transmission[index + 1] = commands[index];
    }

    return i2c_.write(
        address_,
        transmission.data(),
        length + 1
    );
}

}
