#include "graphics/speed_gauge.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace train_controller::graphics
{

namespace
{

constexpr float PI = 3.14159265358979323846f;

constexpr float START_ANGLE_DEGREES = 200.0f;
constexpr float END_ANGLE_DEGREES = 340.0f;

constexpr std::int16_t CENTER_X = 64;
constexpr std::int16_t CENTER_Y = 49;
constexpr std::int16_t OUTER_RADIUS = 40;

constexpr std::uint8_t DIGIT_WIDTH = 5;
constexpr std::uint8_t DIGIT_HEIGHT = 7;
constexpr std::uint8_t DIGIT_SCALE = 3;
constexpr std::int16_t DIGIT_Y = 27;

using DigitBitmap = std::array<std::uint8_t, DIGIT_HEIGHT>;

constexpr std::array<DigitBitmap, 10> DIGITS {{
    {{0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110}},
    {{0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}},
    {{0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}},
    {{0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110}},
    {{0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}},
    {{0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110}},
    {{0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}},
    {{0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}},
    {{0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}},
    {{0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110}}
}};

float degrees_to_radians(float degrees)
{
    return degrees * PI / 180.0f;
}

std::int16_t point_x(float angle_degrees, std::int16_t radius)
{
    const float angle = degrees_to_radians(angle_degrees);

    return static_cast<std::int16_t>(
        std::lround(
            static_cast<float>(CENTER_X) +
            std::cos(angle) * static_cast<float>(radius)
        )
    );
}

std::int16_t point_y(float angle_degrees, std::int16_t radius)
{
    const float angle = degrees_to_radians(angle_degrees);

    return static_cast<std::int16_t>(
        std::lround(
            static_cast<float>(CENTER_Y) +
            std::sin(angle) * static_cast<float>(radius)
        )
    );
}

}

SpeedGauge::SpeedGauge(const SpeedGaugeConfig& config)
    : config_(config)
{
}

void SpeedGauge::draw(
    Framebuffer& framebuffer,
    std::uint16_t speed
) const
{
    const std::uint16_t limited_speed =
        std::min(speed, config_.maximum_speed);

    draw_scale(framebuffer, limited_speed);
    draw_speed_value(framebuffer, limited_speed);
}

void SpeedGauge::draw_scale(
    Framebuffer& framebuffer,
    std::uint16_t speed
) const
{
    if (config_.maximum_speed == 0)
    {
        return;
    }

    const float angle_range =
        END_ANGLE_DEGREES - START_ANGLE_DEGREES;

    const float speed_ratio =
        static_cast<float>(speed) /
        static_cast<float>(config_.maximum_speed);

    const float active_angle =
        START_ANGLE_DEGREES + angle_range * speed_ratio;

    // Draw the complete scale as a thin arc. The active portion is
    // rendered four pixels thick, which remains clear on a 1-bit OLED.
    for (std::int16_t angle =
             static_cast<std::int16_t>(START_ANGLE_DEGREES);
         angle <= static_cast<std::int16_t>(END_ANGLE_DEGREES);
         ++angle)
    {
        const bool active =
            static_cast<float>(angle) <= active_angle;

        const std::int16_t thickness = active ? 4 : 1;

        for (std::int16_t offset = 0;
             offset < thickness;
             ++offset)
        {
            const std::int16_t radius = OUTER_RADIUS - offset;

            framebuffer.set_pixel(
                point_x(static_cast<float>(angle), radius),
                point_y(static_cast<float>(angle), radius)
            );
        }
    }

    if (config_.minor_step == 0)
    {
        return;
    }

    const std::uint32_t maximum_speed =
        config_.maximum_speed;

    const std::uint32_t minor_step =
        config_.minor_step;

    // Draw minor and major tick marks over the arc.
    for (std::uint32_t value = 0;
         value <= maximum_speed;
         value += minor_step)
    {
        const float ratio =
            static_cast<float>(value) /
            static_cast<float>(config_.maximum_speed);

        const float angle =
            START_ANGLE_DEGREES + angle_range * ratio;

        const bool major =
            config_.major_step != 0 &&
            value % config_.major_step == 0;

        const bool active = value <= speed;

        const std::int16_t inner_radius =
            OUTER_RADIUS - (major ? 9 : 5);

        framebuffer.draw_line(
            point_x(angle, inner_radius),
            point_y(angle, inner_radius),
            point_x(angle, OUTER_RADIUS),
            point_y(angle, OUTER_RADIUS)
        );

        if (active)
        {
            framebuffer.draw_line(
                point_x(angle, inner_radius),
                point_y(angle, inner_radius),
                point_x(angle, OUTER_RADIUS - 2),
                point_y(angle, OUTER_RADIUS - 2)
            );
        }

        if (maximum_speed - value < minor_step)
        {
            break;
        }
    }
}

void SpeedGauge::draw_speed_value(
    Framebuffer& framebuffer,
    std::uint16_t speed
) const
{
    std::array<std::uint8_t, 5> digits {};
    std::size_t digit_count = 0;

    if (speed == 0)
    {
        digits[0] = 0;
        digit_count = 1;
    }
    else
    {
        std::uint16_t remaining = speed;

        while (remaining > 0 && digit_count < digits.size())
        {
            digits[digit_count] =
                static_cast<std::uint8_t>(remaining % 10);

            remaining /= 10;
            ++digit_count;
        }

        std::reverse(
            digits.begin(),
            digits.begin() + static_cast<std::ptrdiff_t>(digit_count)
        );
    }

    constexpr std::int16_t scaled_digit_width =
        DIGIT_WIDTH * DIGIT_SCALE;

    constexpr std::int16_t digit_spacing = 2;

    const std::int16_t total_width =
        static_cast<std::int16_t>(
            digit_count * scaled_digit_width +
            (digit_count - 1) * digit_spacing
        );

    std::int16_t x =
        (Framebuffer::WIDTH - total_width) / 2;

    for (std::size_t index = 0;
         index < digit_count;
         ++index)
    {
        draw_digit(
            framebuffer,
            digits[index],
            x,
            DIGIT_Y,
            DIGIT_SCALE
        );

        x += scaled_digit_width + digit_spacing;
    }
}

void SpeedGauge::draw_digit(
    Framebuffer& framebuffer,
    std::uint8_t digit,
    std::int16_t x,
    std::int16_t y,
    std::uint8_t scale
)
{
    if (digit >= DIGITS.size() || scale == 0)
    {
        return;
    }

    for (std::uint8_t row = 0;
         row < DIGIT_HEIGHT;
         ++row)
    {
        for (std::uint8_t column = 0;
             column < DIGIT_WIDTH;
             ++column)
        {
            const std::uint8_t bit =
                static_cast<std::uint8_t>(
                    1u << (DIGIT_WIDTH - 1 - column)
                );

            if ((DIGITS[digit][row] & bit) == 0)
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
                        static_cast<std::int16_t>(
                            x + column * scale + offset_x
                        ),
                        static_cast<std::int16_t>(
                            y + row * scale + offset_y
                        )
                    );
                }
            }
        }
    }
}

}
