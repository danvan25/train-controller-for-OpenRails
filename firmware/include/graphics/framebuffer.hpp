#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace train_controller::graphics
{

class Framebuffer final
{
public:
    static constexpr std::int16_t WIDTH = 128;
    static constexpr std::int16_t HEIGHT = 64;
    static constexpr std::size_t PAGE_COUNT = HEIGHT / 8;
    static constexpr std::size_t BUFFER_SIZE = WIDTH * PAGE_COUNT;

    using Storage = std::array<std::uint8_t, BUFFER_SIZE>;

    Framebuffer();

    void clear(bool enabled = false);

    void set_pixel(
        std::int16_t x,
        std::int16_t y,
        bool enabled = true
    );

    [[nodiscard]] bool get_pixel(
        std::int16_t x,
        std::int16_t y
    ) const;

    void draw_horizontal_line(
        std::int16_t x_start,
        std::int16_t x_end,
        std::int16_t y,
        bool enabled = true
    );

    void draw_vertical_line(
        std::int16_t x,
        std::int16_t y_start,
        std::int16_t y_end,
        bool enabled = true
    );

    void draw_line(
        std::int16_t x_start,
        std::int16_t y_start,
        std::int16_t x_end,
        std::int16_t y_end,
        bool enabled = true
    );

    [[nodiscard]] const std::uint8_t* data() const;
    [[nodiscard]] std::uint8_t* data();
    [[nodiscard]] constexpr std::size_t size() const;

private:
    [[nodiscard]] static constexpr bool coordinates_are_valid(
        std::int16_t x,
        std::int16_t y
    );

    Storage buffer_ {};
};

constexpr std::size_t Framebuffer::size() const
{
    return BUFFER_SIZE;
}

constexpr bool Framebuffer::coordinates_are_valid(
    std::int16_t x,
    std::int16_t y
)
{
    return x >= 0 &&
           x < WIDTH &&
           y >= 0 &&
           y < HEIGHT;
}

}