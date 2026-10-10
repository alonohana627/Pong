#pragma once

#include "match.hpp"

#include <gamelib/core/math.hpp>

#include <array>
#include <cstddef>

namespace pong::app {

// Short visual feedback, purely presentation: it runs on real frame time,
// never touches the match, and the view reads it. A paddle flashes when it
// hits the ball, the screen shakes and the scorer's digits pulse on a point,
// and the ball leaves a short trail.
class Effects {
public:
    static constexpr std::size_t kTrailLength = 10;

    void onPaddleHit(game::Side side);
    void onScore(game::Side scorer);
    void update(float seconds);
    void clear();

    // Records where the ball was drawn this frame, or breaks the trail (serve).
    void recordBall(gamelib::core::Vec2 position, bool inPlay);

    // 1 right after the event, fading to 0.
    [[nodiscard]] float paddleFlash(game::Side side) const { return m_flash[game::index(side)]; }
    [[nodiscard]] float scorePulse(game::Side side) const { return m_pulse[game::index(side)]; }
    // An offset for the whole court; zero once the shake has faded.
    [[nodiscard]] gamelib::core::Vec2 shakeOffset(float time) const;

    // The most recent positions first; `trailCount()` of them are valid.
    [[nodiscard]] const std::array<gamelib::core::Vec2, kTrailLength>& trail() const { return m_trail; }
    [[nodiscard]] std::size_t trailCount() const { return m_trailCount; }

private:
    std::array<float, 2> m_flash{};
    std::array<float, 2> m_pulse{};
    float m_shake = 0.0f;
    std::array<gamelib::core::Vec2, kTrailLength> m_trail{};
    std::size_t m_trailCount = 0;
};

} // namespace pong::app
