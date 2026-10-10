#include "ai.hpp"

#include <algorithm>
#include <cmath>

namespace pong::game {

std::string_view name(const Difficulty difficulty)
{
    switch (difficulty) {
    case Difficulty::Easy: return "EASY";
    case Difficulty::Normal: return "NORMAL";
    case Difficulty::Hard: return "HARD";
    }
    return "NORMAL";
}

float predictY(const gamelib::core::Vec2 position, const gamelib::core::Vec2 velocity, const float targetX)
{
    if (velocity.x == 0.0f) {
        return position.y;
    }
    const float t = (targetX - position.x) / velocity.x;
    const float unfolded = position.y + velocity.y * std::max(t, 0.0f);
    // The ball's center bounces between these two lines. Unfolding the
    // bounces gives a straight line; folding it back is a triangle wave.
    const float top = kWallThickness + kBallRadius;
    const float span = kCourtHeight - 2.0f * (kWallThickness + kBallRadius);
    float m = std::fmod(unfolded - top, 2.0f * span);
    if (m < 0.0f) {
        m += 2.0f * span;
    }
    return top + (m > span ? 2.0f * span - m : m);
}

Ai::Ai(const Side side, const Difficulty difficulty, const std::uint64_t seed)
    : m_side{side}
    , m_difficulty{difficulty}
    , m_rng{seed}
{
}

Ai::Tuning Ai::tuning(const Difficulty difficulty)
{
    switch (difficulty) {
    case Difficulty::Easy: return {.reactionTicks = 26, .maxInput = 0.55f, .aimError = 1.25f, .deadZone = 18.0f};
    case Difficulty::Normal: return {.reactionTicks = 14, .maxInput = 0.75f, .aimError = 0.95f, .deadZone = 12.0f};
    case Difficulty::Hard: return {.reactionTicks = 6, .maxInput = 0.95f, .aimError = 0.75f, .deadZone = 8.0f};
    }
    return tuning(Difficulty::Normal);
}

void Ai::plan(const Match& match)
{
    const Tuning t = tuning(m_difficulty);
    const Ball& ball = match.ball();
    const float paddleX = match.paddle(m_side).rect(m_side).center().x;
    const bool incoming = match.phase() == Phase::Rally
                          && (m_side == Side::Left ? ball.velocity.x < 0.0f : ball.velocity.x > 0.0f);
    if (!incoming) {
        m_target = kCourtHeight / 2.0f; // wait in the middle
        return;
    }
    // A new aim error once per incoming ball. It grows with the ball's speed,
    // so long, fast rallies end in misses even on Hard.
    if (match.rallyHits() != m_plannedRallyHits) {
        m_plannedRallyHits = match.rallyHits();
        const float speedFactor = gamelib::core::length(ball.velocity) / match.rules().serveSpeed;
        const float maxError = t.aimError * speedFactor * (kPaddleSize.y / 2.0f + kBallRadius);
        m_error = m_rng.uniform(-maxError, maxError);
    }
    m_target = predictY(ball.position, ball.velocity, paddleX) + m_error;
}

float Ai::decide(const Match& match)
{
    const Tuning t = tuning(m_difficulty);
    if (--m_ticksToPlan <= 0) {
        plan(match);
        m_ticksToPlan = t.reactionTicks;
    }
    const float distance = m_target - match.paddle(m_side).y;
    if (std::abs(distance) <= t.deadZone) {
        return 0.0f;
    }
    // Full speed when far, slowing down over the last kEaseDistance units so
    // it settles instead of overshooting (independent of the tick rate).
    constexpr float kEaseDistance = 60.0f;
    return std::clamp(distance / kEaseDistance, -t.maxInput, t.maxInput);
}

} // namespace pong::game
