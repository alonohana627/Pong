#include "view.hpp"

#include "palette.hpp"

#include <algorithm>
#include <cmath>
#include <string>

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
constexpr Vec2 kDigitSize{84.0f, 144.0f};
constexpr float kDigitSpacing = 112.0f;
constexpr float kDigitTop = 72.0f;
constexpr float kDigitGlow = 18.0f;
constexpr float kScoreOffset = 190.0f; // digits' center distance from the net

// Menu panel.
constexpr float kPanelWidth = 980.0f; // fits "FULLSCREEN" and "< MEDIUM >" with a gap
constexpr float kTitleText = 14.0f; // canvas units per font pixel
constexpr float kSubtitleText = 6.0f;
constexpr float kItemText = 6.0f;
constexpr float kItemHeight = 72.0f;
constexpr float kPadding = 56.0f;

void _digits(DrawList& list, const Shapes& shapes, const int score, const float centerX, const Color color,
             const float pulse, const Vec2 offset)
{
    const std::string text = std::to_string(std::clamp(score, 0, 99));
    const float width = static_cast<float>(text.size() - 1) * kDigitSpacing;
    const float scale = 1.0f + 0.18f * pulse;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const Vec2 center{centerX - width / 2.0f + static_cast<float>(i) * kDigitSpacing + offset.x,
                          kDigitTop + kDigitSize.y / 2.0f + offset.y};
        const Vec2 size = kDigitSize * scale;
        list.add(Sprite{
            .rect = Rect::fromCenter(center, size + Vec2{2.0f * kDigitGlow, 2.0f * kDigitGlow}),
            .color = color.scaled(1.0f + 0.6f * pulse),
            .color2 = color.withAlpha(0.45f + 0.4f * pulse),
            .shape = shapes.digit,
            .params = {16.0f * scale, kDigitGlow, 0.07f, 0.0f},
            .bits = segmentMask(text[i] - '0'),
        });
    }
}

} // namespace

std::uint32_t segmentMask(const int digit)
{
    //                                     0     1     2     3     4     5     6     7     8     9
    constexpr std::uint32_t kMasks[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    return digit >= 0 && digit <= 9 ? kMasks[digit] : 0x40;
}

void drawCourt(DrawList& list, const Shapes& shapes, const float time)
{
    const Palette& colors = palette();
    list.add(Sprite{
        .rect = {0.0f, 0.0f, kCanvasWidth, kCanvasHeight},
        .color = colors.background,
        .color2 = colors.net,
        .shape = shapes.court,
        .params = {34.0f, 10.0f, 0.0f, 0.0f},
    });
    static_cast<void>(time); // the shader animates with the renderer's time
}

void drawMatch(DrawList& list, const Shapes& shapes, const MatchView& view)
{
    const Palette& colors = palette();
    const game::Match& match = view.match;
    const Vec2 shake = view.effects.shakeOffset(view.time);
    const float fade = view.dim ? 0.45f : 1.0f;

    // Walls: thin glowing bars at the top and bottom.
    for (const float y : {0.0f, kCanvasHeight - game::kWallThickness}) {
        list.box(Rect{0.0f, y, kCanvasWidth, game::kWallThickness}, colors.wall.withAlpha(fade),
                 {.glowWidth = 12.0f, .glowColor = colors.net.withAlpha(0.25f * fade)});
    }

    // Scores.
    const float center = kCanvasWidth / 2.0f;
    _digits(list, shapes, match.score(game::Side::Left), center - kScoreOffset, colors.leftPaddle.withAlpha(fade),
            view.effects.scorePulse(game::Side::Left), shake);
    _digits(list, shapes, match.score(game::Side::Right), center + kScoreOffset, colors.rightPaddle.withAlpha(fade),
            view.effects.scorePulse(game::Side::Right), shake);
    const gamelib::draw::TextStyle nameStyle{.pixelSize = 4.0f, .color = colors.textDim.withAlpha(fade), .align = Align::Center};
    const float nameY = kDigitTop + kDigitSize.y + 34.0f;
    list.text(view.leftName, center - kScoreOffset + shake.x, nameY + shake.y, nameStyle);
    list.text(view.rightName, center + kScoreOffset + shake.x, nameY + shake.y, nameStyle);

    // Paddles, interpolated between ticks, flashing on a hit.
    for (const game::Side side : {game::Side::Left, game::Side::Right}) {
        const float y = match.paddle(side).y.at(view.alpha);
        game::Paddle drawn = match.paddle(side);
        drawn.y.snap(y);
        const float flash = view.effects.paddleFlash(side);
        const Color base = side == game::Side::Left ? colors.leftPaddle : colors.rightPaddle;
        Rect rect = drawn.rect(side);
        rect.x += shake.x;
        rect.y += shake.y;
        list.box(rect, gamelib::core::mix(base, Color{1.0f, 1.0f, 1.0f, 1.0f}, 0.6f * flash).withAlpha(fade),
                 {.cornerRadius = 10.0f, .glowWidth = 22.0f + 18.0f * flash, .glowColor = base.withAlpha((0.45f + 0.4f * flash) * fade)});
    }

    // The ball and its trail, only while it is in play.
    if (match.phase() == game::Phase::Rally) {
        const auto& trail = view.effects.trail();
        for (std::size_t i = view.effects.trailCount(); i-- > 1;) {
            const float age = static_cast<float>(i) / static_cast<float>(Effects::kTrailLength);
            list.circle(trail[i] + shake, game::kBallRadius * (1.0f - 0.6f * age),
                        colors.ballGlow.withAlpha(0.35f * (1.0f - age) * fade));
        }
        const Vec2 position = match.ball().position.at(view.alpha);
        list.circle(position + shake, game::kBallRadius, colors.ball.withAlpha(fade),
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

void drawMenu(DrawList& list, const MenuView& view)
{
    const Palette& colors = palette();
    const Menu& menu = view.menu;
    list.box({0.0f, 0.0f, kCanvasWidth, kCanvasHeight}, colors.veil);

    const auto itemCount = static_cast<float>(menu.items().size());
    const float titleHeight = gamelib::draw::textHeight(kTitleText);
    const float subtitleHeight = view.subtitle.empty() ? 0.0f : gamelib::draw::textHeight(kSubtitleText) + 28.0f;
    const float height = kPadding + titleHeight + 40.0f + subtitleHeight + itemCount * kItemHeight + kPadding;
    const Rect panel{(kCanvasWidth - kPanelWidth) / 2.0f, (kCanvasHeight - height) / 2.0f, kPanelWidth, height};
    list.box(panel, colors.panel,
             {.cornerRadius = 22.0f, .borderWidth = 4.0f, .borderColor = colors.panelBorder, .glowWidth = 30.0f});

    const float centerX = panel.center().x;
    float y = panel.y + kPadding;
    list.text(view.title, centerX, y, {.pixelSize = kTitleText, .color = colors.accent, .align = Align::Center, .shadow = true});
    y += titleHeight + 40.0f;
    if (!view.subtitle.empty()) {
        list.text(view.subtitle, centerX, y, {.pixelSize = kSubtitleText, .color = colors.text, .align = Align::Center});
        y += subtitleHeight;
    }

    const float itemTextHeight = gamelib::draw::textHeight(kItemText);
    for (std::size_t i = 0; i < menu.items().size(); ++i) {
        const MenuItem item = menu.items()[i];
        const bool selected = i == menu.selected();
        const float textY = y + (kItemHeight - itemTextHeight) / 2.0f;
        if (selected) {
            list.box({panel.x + 24.0f, y + 6.0f, panel.width - 48.0f, kItemHeight - 12.0f}, colors.selection,
                     {.cornerRadius = 12.0f});
            list.text(">", panel.x + 52.0f, textY, {.pixelSize = kItemText, .color = colors.accent});
        }
        const Color labelColor = selected ? colors.accent : colors.text;
        list.text(label(item), panel.x + 96.0f, textY, {.pixelSize = kItemText, .color = labelColor});
        if (const std::string shown = value(item, menu.settings()); !shown.empty()) {
            list.text(selected ? "< " + shown + " >" : shown, panel.right() - 52.0f, textY,
                      {.pixelSize = kItemText, .color = labelColor, .align = Align::Right});
        }
        y += kItemHeight;
    }

    if (!view.footer.empty()) {
        list.text(view.footer, kCanvasWidth / 2.0f, kCanvasHeight - 90.0f,
                  {.pixelSize = 4.0f, .color = colors.textDim, .align = Align::Center});
    }
}

} // namespace pong::app
