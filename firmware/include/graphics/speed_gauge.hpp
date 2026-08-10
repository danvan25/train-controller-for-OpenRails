#pragma once

#include <cstdint>

#include "graphics/framebuffer.hpp"

namespace train_controller::graphics
{

struct SpeedGaugeConfig
{
    std::uint16_t maximum_speed;
    std::uint16_t major_step;
    std::uint16_t minor_step;
};

class SpeedGauge final
{
public:
    explicit SpeedGauge(const SpeedGaugeConfig& config);

    void draw(
        Framebuffer& framebuffer,
        std::uint16_t speed
    ) const;

private:
    void draw_scale(
        Framebuffer& framebuffer,
        std::uint16_t speed
    ) const;

    void draw_speed_value(
        Framebuffer& framebuffer,
        std::uint16_t speed
    ) const;

    static void draw_digit(
        Framebuffer& framebuffer,
        std::uint8_t digit,
        std::int16_t x,
        std::int16_t y,
        std::uint8_t scale
    );

    SpeedGaugeConfig config_;
};

}
