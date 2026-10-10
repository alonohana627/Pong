#pragma once

#include "effects.hpp"
#include "match.hpp"
#include "menu.hpp"

#include <gamelib/draw/draw_list.hpp>

#include <cstdint>
#include <string_view>

namespace pong::app {

// Pong's own procedural shapes, registered with the renderer at startup.
struct Shapes {
    gamelib::draw::ShapeId court;
    gamelib::draw::ShapeId digit;
};

// The seven-segment pattern of a decimal digit (0-9): bits 0..6 are segments
// a..g, as digit.frag expects. Anything else shows a dash (segment g).
[[nodiscard]] std::uint32_t segmentMask(int digit);

struct MatchView {
    const game::Match& match;
    const Effects& effects;
    float alpha = 1.0f; // interpolation between the previous and current tick
    float time = 0.0f;
    std::string_view leftName;
    std::string_view rightName;
    bool dim = false; // behind a menu: fainter, so the menu stands out
};

// Views turn state into sprites on the 1920x1080 canvas. They read the
// match, the menu and the effects, never change them.
void drawCourt(gamelib::draw::DrawList& list, const Shapes& shapes, float time);
void drawMatch(gamelib::draw::DrawList& list, const Shapes& shapes, const MatchView& view);

struct MenuView {
    const Menu& menu;
    std::string_view title;     // "PONG", "PAUSED", "YOU WIN!"...
    std::string_view subtitle;  // a line under the title, may be empty
    std::string_view footer;    // help text at the bottom of the screen, may be empty
};
void drawMenu(gamelib::draw::DrawList& list, const MenuView& view);

// "GET READY" plus the receiving side's arrow while a serve is pending.
void drawServeHint(gamelib::draw::DrawList& list, const game::Match& match, float time);

} // namespace pong::app
