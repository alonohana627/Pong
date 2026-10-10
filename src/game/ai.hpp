#pragma once

#include "match.hpp"

#include <gamelib/core/rng.hpp>

#include <cstdint>
#include <string_view>

namespace pong::game {

enum class Difficulty : std::uint8_t { Easy, Normal, Hard };
inline constexpr int kDifficultyCount = 3;

[[nodiscard]] std::string_view name(Difficulty difficulty); // "EASY", "NORMAL", "HARD"

// Where a ball at `position` moving with `velocity` will be when it reaches
// x = targetX, bouncing off the top and bottom walls on the way. Assumes it
// is moving towards targetX.
[[nodiscard]] float predictY(gamelib::core::Vec2 position, gamelib::core::Vec2 velocity, float targetX);

// A beatable computer opponent. It predicts where the ball will cross its
// paddle's line, but only re-plans every so often (reaction time), aims with
// a deliberate error that grows with the ball's speed, and moves at a capped
// speed - all scaled by difficulty. When the ball moves away it drifts back
// to the middle. Deterministic for a given seed.
class Ai {
public:
    Ai(Side side, Difficulty difficulty, std::uint64_t seed);

    // The paddle input for this tick, -1..1 (see Input).
    [[nodiscard]] float decide(const Match& match);

    [[nodiscard]] Side side() const { return m_side; }
    [[nodiscard]] Difficulty difficulty() const { return m_difficulty; }
    // The y the paddle is currently heading for.
    [[nodiscard]] float target() const { return m_target; }

private:
    struct Tuning {
        int reactionTicks;   // ticks between re-plans
        float maxInput;      // fraction of the paddle speed it uses
        float aimError;      // max miss, in paddle half-heights, at serve speed
        float deadZone;      // units: close enough, stop jittering
    };
    [[nodiscard]] static Tuning tuning(Difficulty difficulty);

    void plan(const Match& match);

    Side m_side;
    Difficulty m_difficulty;
    gamelib::core::Rng m_rng;
    float m_target = kCourtHeight / 2.0f;
    float m_error = 0.0f;      // this rally's aim error, in units
    int m_ticksToPlan = 0;
    int m_plannedRallyHits = -1; // the rally hit count the error was chosen for
};

} // namespace pong::game
