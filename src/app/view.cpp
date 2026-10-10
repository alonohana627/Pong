#include "view.hpp"

#include "palette.hpp"

#include <algorithm>
#include <cmath>

namespace pong::app {

using gamelib::core::Color;
using gamelib::core::Rect;
using gamelib::core::Vec2;
using gamelib::draw::Align;
using gamelib::draw::DrawList;
using gamelib::draw::Sprite;

namespace {

constexpr float kCanvasWidth = game::kCourtWidth;
constexpr float kCanvasHeight = game::kCourtHeight;

// Score digits.
constexpr float kDigitHeight = 144.0f;
constexpr float kDigitTop = 72.0f;
constexpr float kDigitGlow = 18.0f;
constexpr float kScoreOffset = 190.0f; // digits' center distance from the net

void _score(DrawList& list, const int score, const float centerX, const Color color, const float pulse,
            const Vec2 offset)
{
    // A point makes the scorer's digits swell and brighten for a moment.
    const float height = kDigitHeight * (1.0f + 0.18f * pulse);
    list.number(std::clamp(score, 0, 99), centerX + offset.x, kDigitTop + (kDigitHeight - height) / 2.0f + offset.y,
                {.height = height,
                 .color = color.scaled(1.0f + 0.6f * pulse),
                 .glowColor = color.withAlpha((0.45f + 0.4f * pulse) * color.a),
                 .glowWidth = kDigitGlow,
                 .align = Align::Center});
}

} // namespace

void drawCourt(DrawList& list, const Shapes& shapes)
{
    const Palette& colors = palette();
    list.add(Sprite{
        .rect = {0.0f, 0.0f, kCanvasWidth, kCanvasHeight},
        .color = colors.background,
        .color2 = colors.net,
        .shape = shapes.court,
        .params = {34.0f, 10.0f, 0.0f, 0.0f}, // the shader animates with the renderer's time
    });
}

void drawMatch(DrawList& list, const MatchView& view)
{
    const Palette& colors = palette();
    const game::Match& match = view.match;
    const Effects& effects = view.effects;
    const Vec2 shake = effects.shake.offset(view.time);
    const float fade = view.dim ? 0.45f : 1.0f;

    // Walls: thin glowing bars at the top and bottom.
    for (const float y : {0.0f, kCanvasHeight - game::kWallThickness}) {
        list.box(Rect{0.0f, y, kCanvasWidth, game::kWallThickness}, colors.wall.withAlpha(fade),
                 {.glowWidth = 12.0f, .glowColor = colors.net.withAlpha(0.25f * fade)});
    }

    // Scores.
    const float center = kCanvasWidth / 2.0f;
    _score(list, match.score(game::Side::Left), center - kScoreOffset, colors.leftPaddle.withAlpha(fade),
           effects.scorePulse[0].value(), shake);
    _score(list, match.score(game::Side::Right), center + kScoreOffset, colors.rightPaddle.withAlpha(fade),
           effects.scorePulse[1].value(), shake);
    const gamelib::draw::TextStyle nameStyle{.pixelSize = 4.0f, .color = colors.textDim.withAlpha(fade), .align = Align::Center};
    const float nameY = kDigitTop + kDigitHeight + 34.0f;
    list.text(view.leftName, center - kScoreOffset + shake.x, nameY + shake.y, nameStyle);
    list.text(view.rightName, center + kScoreOffset + shake.x, nameY + shake.y, nameStyle);

    // Paddles, interpolated between ticks, flashing on a hit.
    for (const game::Side side : {game::Side::Left, game::Side::Right}) {
        game::Paddle drawn = match.paddle(side);
        drawn.y.snap(match.paddle(side).y.at(view.alpha));
        const float flash = effects.paddleFlash[game::index(side)].value();
        const Color base = side == game::Side::Left ? colors.leftPaddle : colors.rightPaddle;
        Rect rect = drawn.rect(side);
        rect.x += shake.x;
        rect.y += shake.y;
        list.box(rect, gamelib::core::mix(base, Color{1.0f, 1.0f, 1.0f, 1.0f}, 0.6f * flash).withAlpha(fade),
                 {.cornerRadius = 10.0f, .glowWidth = 22.0f + 18.0f * flash, .glowColor = base.withAlpha((0.45f + 0.4f * flash) * fade)});
    }

    // The ball and its trail, only while it is in play.
    if (match.phase() == game::Phase::Rally) {
        for (std::size_t i = effects.trail.size(); i-- > 1;) {
            const float age = gamelib::fx::Trail<Effects::kTrailLength>::age(i);
            list.circle(effects.trail[i] + shake, game::kBallRadius * (1.0f - 0.6f * age),
                        colors.ballGlow.withAlpha(0.35f * (1.0f - age) * fade));
        }
        list.circle(match.ball().position.at(view.alpha) + shake, game::kBallRadius, colors.ball.withAlpha(fade),
                    {.glowWidth = 26.0f, .glowColor = colors.ballGlow.withAlpha(colors.ballGlow.a * fade)});
    }
}

void drawServeHint(DrawList& list, const game::Match& match, const float time)
{
    if (match.phase() != game::Phase::Serving) {
        return;
    }
    const Palette& colors = palette();
    const Vec2 center{kCanvasWidth / 2.0f, kCanvasHeight / 2.0f};
    // A ring closing in on the ball's start as the serve approaches.
    const float progress = match.serveProgress();
    list.circle(center, gamelib::core::lerp(90.0f, game::kBallRadius + 4.0f, progress), colors.ballGlow.withAlpha(0.8f),
                {.ringWidth = 4.0f, .glowWidth = 12.0f, .glowColor = colors.ballGlow.withAlpha(0.3f)});
    list.circle(center, game::kBallRadius, colors.ball.withAlpha(0.5f + 0.5f * progress));
    // An arrow towards whoever receives the serve.
    const float direction = match.receiver() == game::Side::Left ? -1.0f : 1.0f;
    const float bob = 6.0f * std::sin(time * 8.0f);
    list.text(direction < 0.0f ? "<" : ">", center.x + direction * (140.0f + bob), center.y - 21.0f,
              {.pixelSize = 6.0f, .color = colors.accent, .align = Align::Center});
}

} // namespace pong::app
