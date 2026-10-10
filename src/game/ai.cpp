#include "ai.hpp"

namespace pong::game {

gamelib::ai::Skill skill(const Difficulty difficulty)
{
    switch (difficulty) {
    case Difficulty::Easy:
        return {.reactionTicks = 26, .maxInput = 0.55f, .aimError = 1.25f, .deadZone = 18.0f, .easeDistance = 60.0f};
    case Difficulty::Normal:
        return {.reactionTicks = 14, .maxInput = 0.75f, .aimError = 0.95f, .deadZone = 12.0f, .easeDistance = 60.0f};
    case Difficulty::Hard:
        return {.reactionTicks = 6, .maxInput = 0.95f, .aimError = 0.75f, .deadZone = 8.0f, .easeDistance = 60.0f};
    }
    return skill(Difficulty::Normal);
}

float predictY(const gamelib::core::Vec2 position, const gamelib::core::Vec2 velocity, const float targetX)
{
    // The ball's center bounces between these two lines.
    return gamelib::ai::predictBounce(position, velocity, targetX, kWallThickness + kBallRadius,
                                      kCourtHeight - kWallThickness - kBallRadius);
}

Ai::Ai(const Side side, const Difficulty difficulty, const std::uint64_t seed)
    : m_side{side}
    , m_difficulty{difficulty}
    , m_skill{skill(difficulty)}
    , m_rng{seed}
    , m_reaction{m_skill.reactionTicks}
{
}

void Ai::plan(const Match& match)
{
    const Ball& ball = match.ball();
    const float paddleX = match.paddle(m_side).rect(m_side).center().x;
    const bool incoming = match.phase() == Phase::Rally
                          && (m_side == Side::Left ? ball.velocity.x < 0.0f : ball.velocity.x > 0.0f);
    if (!incoming) {
        m_target = kCourtHeight / 2.0f; // wait in the middle
        return;
    }
    // One aim error per incoming ball (the rally hit count identifies it). It
    // grows with the ball's speed, so long, fast rallies end in misses even on Hard.
    const float speedFactor = gamelib::core::length(ball.velocity) / match.rules().serveSpeed;
    const float maxError = m_skill.aimError * speedFactor * (kPaddleSize.y / 2.0f + kBallRadius);
    const float error = m_error.update(match.rallyHits(), maxError, m_rng);
    m_target = predictY(ball.position.value(), ball.velocity, paddleX) + error;
}

float Ai::decide(const Match& match)
{
    if (m_reaction.tick()) {
        plan(match);
    }
    return gamelib::ai::steer(match.paddle(m_side).y.value(), m_target, m_skill);
}

} // namespace pong::game
