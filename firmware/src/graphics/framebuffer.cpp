#include "graphics/framebuffer.hpp"

#include <algorithm>
#include <cstdlib>

namespace train_controller::graphics
{

Framebuffer::Framebuffer()
{
    clear();
}

void Framebuffer::clear(bool enabled)
{
    buffer_.fill(enabled ? 0xFF : 0x00);
}

void Framebuffer::set_pixel(
    std::int16_t x,
    std::int16_t y,
    bool enabled
)
{
    if (!coordinates_are_valid(x, y))
    {
        return;
    }

    const std::size_t page =
        static_cast<std::size_t>(y) / 8;

    const std::size_t index =
        page * static_cast<std::size_t>(WIDTH) +
        static_cast<std::size_t>(x);

    const std::uint8_t mask =
        static_cast<std::uint8_t>(
            1u << (static_cast<std::uint8_t>(y) & 0x07u)
        );

    if (enabled)
    {
        buffer_[index] |= mask;
    }
    else
    {
        buffer_[index] &= static_cast<std::uint8_t>(~mask);
    }
}

bool Framebuffer::get_pixel(
    std::int16_t x,
    std::int16_t y
) const
{
    if (!coordinates_are_valid(x, y))
    {
        return false;
    }

    const std::size_t page =
        static_cast<std::size_t>(y) / 8;

    const std::size_t index =
        page * static_cast<std::size_t>(WIDTH) +
        static_cast<std::size_t>(x);

    const std::uint8_t mask =
        static_cast<std::uint8_t>(
            1u << (static_cast<std::uint8_t>(y) & 0x07u)
        );

    return (buffer_[index] & mask) != 0;
}

void Framebuffer::draw_horizontal_line(
    std::int16_t x_start,
    std::int16_t x_end,
    std::int16_t y,
    bool enabled
)
{
    if (y < 0 || y >= HEIGHT)
    {
        return;
    }

    if (x_start > x_end)
    {
        std::swap(x_start, x_end);
    }

    x_start = std::max<std::int16_t>(x_start, 0);
    x_end = std::min<std::int16_t>(x_end, WIDTH - 1);

    for (std::int16_t x = x_start; x <= x_end; ++x)
    {
        set_pixel(x, y, enabled);
    }
}

void Framebuffer::draw_vertical_line(
    std::int16_t x,
    std::int16_t y_start,
    std::int16_t y_end,
    bool enabled
)
{
    if (x < 0 || x >= WIDTH)
    {
        return;
    }

    if (y_start > y_end)
    {
        std::swap(y_start, y_end);
    }

    y_start = std::max<std::int16_t>(y_start, 0);
    y_end = std::min<std::int16_t>(y_end, HEIGHT - 1);

    for (std::int16_t y = y_start; y <= y_end; ++y)
    {
        set_pixel(x, y, enabled);
    }
}

void Framebuffer::draw_line(
    std::int16_t x_start,
    std::int16_t y_start,
    std::int16_t x_end,
    std::int16_t y_end,
    bool enabled
)
{
    std::int32_t x = x_start;
    std::int32_t y = y_start;

    const std::int32_t delta_x =
        std::abs(
            static_cast<std::int32_t>(x_end) -
            static_cast<std::int32_t>(x_start)
        );

    const std::int32_t delta_y =
        -std::abs(
            static_cast<std::int32_t>(y_end) -
            static_cast<std::int32_t>(y_start)
        );

    const std::int32_t step_x =
        x_start < x_end ? 1 : -1;

    const std::int32_t step_y =
        y_start < y_end ? 1 : -1;

    std::int32_t error = delta_x + delta_y;

    while (true)
    {
        set_pixel(
            static_cast<std::int16_t>(x),
            static_cast<std::int16_t>(y),
            enabled
        );

        if (x == x_end && y == y_end)
        {
            break;
        }

        const std::int32_t doubled_error = error * 2;

        if (doubled_error >= delta_y)
        {
            error += delta_y;
            x += step_x;
        }

        if (doubled_error <= delta_x)
        {
            error += delta_x;
            y += step_y;
        }
    }
}

const std::uint8_t* Framebuffer::data() const
{
    return buffer_.data();
}

std::uint8_t* Framebuffer::data()
{
    return buffer_.data();
}

}