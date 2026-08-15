#include "graphics/evm_display.hpp"

#include <array>
#include <cstddef>

namespace train_controller::graphics
{

namespace
{

constexpr std::uint8_t CHARACTER_WIDTH = 5;
constexpr std::uint8_t CHARACTER_HEIGHT = 7;
constexpr std::uint8_t CHARACTER_SPACING = 1;

using CharacterBitmap =
    std::array<std::uint8_t, CHARACTER_HEIGHT>;

constexpr CharacterBitmap BLANK {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};

constexpr CharacterBitmap DIGIT_0 {
    0b01110,
    0b10001,
    0b10011,
    0b10101,
    0b11001,
    0b10001,
    0b01110
};

constexpr CharacterBitmap DIGIT_1 {
    0b00100,
    0b01100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b01110
};

constexpr CharacterBitmap DIGIT_2 {
    0b01110,
    0b10001,
    0b00001,
    0b00010,
    0b00100,
    0b01000,
    0b11111
};

constexpr CharacterBitmap DIGIT_4 {
    0b00010,
    0b00110,
    0b01010,
    0b10010,
    0b11111,
    0b00010,
    0b00010
};

constexpr CharacterBitmap DIGIT_8 {
    0b01110,
    0b10001,
    0b10001,
    0b01110,
    0b10001,
    0b10001,
    0b01110
};

constexpr CharacterBitmap LETTER_A {
    0b01110,
    0b10001,
    0b10001,
    0b11111,
    0b10001,
    0b10001,
    0b10001
};

constexpr CharacterBitmap LETTER_E {
    0b11111,
    0b10000,
    0b10000,
    0b11110,
    0b10000,
    0b10000,
    0b11111
};

constexpr CharacterBitmap LETTER_M {
    0b10001,
    0b11011,
    0b10101,
    0b10101,
    0b10001,
    0b10001,
    0b10001
};

constexpr CharacterBitmap LETTER_O {
    0b01110,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b01110
};

constexpr CharacterBitmap LETTER_P {
    0b11110,
    0b10001,
    0b10001,
    0b11110,
    0b10000,
    0b10000,
    0b10000
};

constexpr CharacterBitmap LETTER_S {
    0b01111,
    0b10000,
    0b10000,
    0b01110,
    0b00001,
    0b00001,
    0b11110
};

constexpr CharacterBitmap LETTER_T {
    0b11111,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100
};

constexpr CharacterBitmap LETTER_V {
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b01010,
    0b00100
};

constexpr CharacterBitmap LETTER_X {
    0b10001,
    0b10001,
    0b01010,
    0b00100,
    0b01010,
    0b10001,
    0b10001
};

constexpr CharacterBitmap MINUS {
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b00000,
    0b00000,
    0b00000
};

const CharacterBitmap& get_character_bitmap(char character)
{
    switch (character)
    {
        case '0': return DIGIT_0;
        case '1': return DIGIT_1;
        case '2': return DIGIT_2;
        case '4': return DIGIT_4;
        case '8': return DIGIT_8;

        case 'A': return LETTER_A;
        case 'E': return LETTER_E;
        case 'M': return LETTER_M;
        case 'O': return LETTER_O;
        case 'P': return LETTER_P;
        case 'S': return LETTER_S;
        case 'T': return LETTER_T;
        case 'V': return LETTER_V;
        case 'X': return LETTER_X;

        case '-': return MINUS;
        case ' ': return BLANK;

        default: return BLANK;
    }
}

}

void EvmDisplay::draw(
    Framebuffer& framebuffer,
    EvmSignal signal
) const
{
    framebuffer.clear();

    // Outer frame.
    framebuffer.draw_horizontal_line(0, 127, 0);
    framebuffer.draw_horizontal_line(0, 127, 63);
    framebuffer.draw_vertical_line(0, 0, 63);
    framebuffer.draw_vertical_line(127, 0, 63);

    // Header.
    draw_text(framebuffer, "EVM", 5, 4, 1);
    framebuffer.draw_horizontal_line(4, 123, 14);

    switch (signal)
    {
        case EvmSignal::MaximumSpeed:
            draw_centered_text(
                framebuffer,
                "MAX",
                25,
                3
            );
            break;

        case EvmSignal::Speed120:
            draw_centered_text(
                framebuffer,
                "120",
                25,
                3
            );
            break;

        case EvmSignal::Speed80:
            draw_centered_text(
                framebuffer,
                "80",
                25,
                3
            );
            break;

        case EvmSignal::Speed40:
            draw_centered_text(
                framebuffer,
                "40",
                25,
                3
            );
            break;

        case EvmSignal::PrepareToStop:
            draw_centered_text(
                framebuffer,
                "0",
                25,
                3
            );
            break;

        case EvmSignal::PassedStopSignal:
            draw_centered_text(
                framebuffer,
                "STOP",
                25,
                3
            );
            break;

        case EvmSignal::NoSignal:
            draw_centered_text(
                framebuffer,
                "---",
                25,
                3
            );
            break;

        case EvmSignal::Shunting:
            draw_centered_text(
                framebuffer,
                "T 40",
                25,
                3
            );
            break;
    }
}

void EvmDisplay::draw_centered_text(
    Framebuffer& framebuffer,
    std::string_view text,
    std::int16_t y,
    std::uint8_t scale
)
{
    if (text.empty() || scale == 0)
    {
        return;
    }

    const std::int16_t unscaled_width =
        static_cast<std::int16_t>(
            text.size() * CHARACTER_WIDTH +
            (text.size() - 1) * CHARACTER_SPACING
        );

    const std::int16_t scaled_width =
        unscaled_width * scale;

    const std::int16_t x =
        (Framebuffer::WIDTH - scaled_width) / 2;

    draw_text(
        framebuffer,
        text,
        x,
        y,
        scale
    );
}

void EvmDisplay::draw_text(
    Framebuffer& framebuffer,
    std::string_view text,
    std::int16_t x,
    std::int16_t y,
    std::uint8_t scale
)
{
    if (scale == 0)
    {
        return;
    }

    const std::int16_t character_advance =
        (CHARACTER_WIDTH + CHARACTER_SPACING) * scale;

    for (std::size_t index = 0;
         index < text.size();
         ++index)
    {
        draw_character(
            framebuffer,
            text[index],
            x + static_cast<std::int16_t>(index) *
                    character_advance,
            y,
            scale
        );
    }
}

void EvmDisplay::draw_character(
    Framebuffer& framebuffer,
    char character,
    std::int16_t x,
    std::int16_t y,
    std::uint8_t scale
)
{
    if (scale == 0)
    {
        return;
    }

    const CharacterBitmap& bitmap =
        get_character_bitmap(character);

    for (std::uint8_t row = 0;
         row < CHARACTER_HEIGHT;
         ++row)
    {
        for (std::uint8_t column = 0;
             column < CHARACTER_WIDTH;
             ++column)
        {
            const std::uint8_t mask =
                1u << (CHARACTER_WIDTH - 1 - column);

            if ((bitmap[row] & mask) == 0)
            {
                continue;
            }

            for (std::uint8_t offset_y = 0;
                 offset_y < scale;
                 ++offset_y)
            {
                for (std::uint8_t offset_x = 0;
                     offset_x < scale;
                     ++offset_x)
                {
                    framebuffer.set_pixel(
                        x + column * scale + offset_x,
                        y + row * scale + offset_y
                    );
                }
            }
        }
    }
}

}