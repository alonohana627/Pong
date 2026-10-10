#include "effects.hpp"

#include <algorithm>
#include <cmath>

namespace pong::app {

namespace {

constexpr float kFlashSeconds = 0.25f;
constexpr float kPulseSeconds = 0.6f;
constexpr float kShakeSeconds = 0.35f;
constexpr float kShakeAmplitude = 14.0f; // canvas units at full strength

} // namespace

void Effects::onPaddleHit(const game::Side side)
{
    m_flash[game::index(side)] = 1.0f;
}

void Effects::onScore(const game::Side scorer)
{
    m_pulse[game::index(scorer)] = 1.0f;
    m_shake = 1.0f;
}

void Effects::update(const float seconds)
{
    for (float& flash : m_flash) {
        flash = std::max(0.0f, flash - seconds / kFlashSeconds);
    }
    for (float& pulse : m_pulse) {
        pulse = std::max(0.0f, pulse - seconds / kPulseSeconds);
    }
    m_shake = std::max(0.0f, m_shake - seconds / kShakeSeconds);
}

void Effects::clear()
{
    *this = Effects{};
}

void Effects::recordBall(const gamelib::core::Vec2 position, const bool inPlay)
{
    if (!inPlay) {
        m_trailCount = 0;
        return;
    }
    std::shift_right(m_trail.begin(), m_trail.end(), 1);
    m_trail[0] = position;
    m_trailCount = std::min(m_trailCount + 1, kTrailLength);
}

gamelib::core::Vec2 Effects::shakeOffset(const float time) const
{
    if (m_shake <= 0.0f) {
        return {};
    }
    // Two unrelated fast sines: a jitter that looks random but is smooth.
    const float strength = kShakeAmplitude * m_shake * m_shake;
    return {std::sin(time * 71.0f) * strength, std::cos(time * 53.0f) * strength};
}

} // namespace pong::app
