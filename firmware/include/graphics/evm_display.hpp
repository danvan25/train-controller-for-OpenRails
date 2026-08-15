#pragma once

#include <cstdint>
#include <string_view>

#include "graphics/framebuffer.hpp"

namespace train_controller::graphics
{

enum class EvmSignal : std::uint8_t
{
    MaximumSpeed,
    Speed120,
    Speed80,
    Speed40,
    PrepareToStop,
    PassedStopSignal,
    NoSignal,
    Shunting
};

class EvmDisplay final
{
public:
    void draw(
        Framebuffer& framebuffer,
        EvmSignal signal
    ) const;

private:
    static void draw_centered_text(
        Framebuffer& framebuffer,
        std::string_view text,
        std::int16_t y,
        std::uint8_t scale
    );

    static void draw_text(
        Framebuffer& framebuffer,
        std::string_view text,
        std::int16_t x,
        std::int16_t y,
        std::uint8_t scale
    );

    static void draw_character(
        Framebuffer& framebuffer,
        char character,
        std::int16_t x,
        std::int16_t y,
        std::uint8_t scale
    );
};

}