#pragma once

#include <gamelib/core/color.hpp>

namespace pong::app {

// Every color Pong uses, authored as sRGB hex and converted to linear once.
struct Palette {
    gamelib::core::Color background;
    gamelib::core::Color net;
    gamelib::core::Color wall;
    gamelib::core::Color leftPaddle;
    gamelib::core::Color rightPaddle;
    gamelib::core::Color ball;
    gamelib::core::Color ballGlow;
    gamelib::core::Color text;
    gamelib::core::Color textDim;
    gamelib::core::Color accent;
    gamelib::core::Color panel;
    gamelib::core::Color panelBorder;
    gamelib::core::Color selection;
    gamelib::core::Color veil;
};

[[nodiscard]] inline const Palette& palette()
{
    using gamelib::core::srgb;
    static const Palette kPalette{
        .background = srgb(0x0E1428),
        .net = srgb(0x5B7BD5),
        .wall = srgb(0x1C2850),
        .leftPaddle = srgb(0x31C7EF),
        .rightPaddle = srgb(0xFF5E8A),
        .ball = srgb(0xFFF6E0),
        .ballGlow = srgb(0xFFD166, 0.8f),
        .text = srgb(0xF2F0E5),
        .textDim = srgb(0x8B93AF),
        .accent = srgb(0xFFD166),
        .panel = srgb(0x121833, 0.96f),
        .panelBorder = srgb(0x3A4A86),
        .selection = srgb(0x26305A),
        .veil = {0.0f, 0.0f, 0.0f, 0.55f},
    };
    return kPalette;
}

} // namespace pong::app
