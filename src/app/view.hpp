#pragma once

#include "effects.hpp"
#include "match.hpp"

#include <gamelib/draw/draw_list.hpp>

#include <string_view>

namespace pong::app {

// Pong's own procedural shape, registered with the renderer at startup. The
// score digits use GameLib's built-in seven-segment shape.
struct Shapes {
    gamelib::draw::ShapeId court;
};

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
// match and the effects, never change them. Menus are drawn by GameLib's
// ui::drawMenu.
void drawCourt(gamelib::draw::DrawList& list, const Shapes& shapes);
void drawMatch(gamelib::draw::DrawList& list, const MatchView& view);

// A closing ring plus the receiving side's arrow while a serve is pending.
void drawServeHint(gamelib::draw::DrawList& list, const game::Match& match, float time);

} // namespace pong::app
