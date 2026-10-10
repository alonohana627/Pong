#pragma once

#include "match.hpp"

#include <gamelib/ai/opponent.hpp>
#include <gamelib/core/options.hpp>
#include <gamelib/core/rng.hpp>

#include <cstdint>
#include <string_view>

namespace pong::game {

enum class Difficulty : std::uint8_t { Easy, Normal, Hard };
inline constexpr gamelib::core::OptionSet<Difficulty, 3> kDifficulties{{"EASY", "NORMAL", "HARD"}};

[[nodiscard]] inline std::string_view name(const Difficulty difficulty) { return kDifficulties.name(difficulty); }

// How well each difficulty plays (GameLib's ai::Skill; aimError is in
// paddle half-heights at serve speed).
[[nodiscard]] gamelib::ai::Skill skill(Difficulty difficulty);

// Where a ball at `position` moving with `velocity` will be when it reaches
// x = targetX, bouncing off the top and bottom walls on the way.
[[nodiscard]] float predictY(gamelib::core::Vec2 position, gamelib::core::Vec2 velocity, float targetX);

// A beatable computer opponent, composed from GameLib's ai helpers: it
// predicts where the ball will cross its paddle's line (ai::predictBounce),
// re-plans only every few ticks (ai::ReactionTimer), aims with one error per
// incoming ball that grows with the ball's speed (ai::AimError), and steers
// with a dead zone and capped strength (ai::steer). When the ball moves away
// it drifts back to the middle. Deterministic for a given seed.
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
    void plan(const Match& match);

    Side m_side;
    Difficulty m_difficulty;
    gamelib::ai::Skill m_skill;
    gamelib::core::Rng m_rng;
    gamelib::ai::ReactionTimer m_reaction;
    gamelib::ai::AimError m_error;
    float m_target = kCourtHeight / 2.0f;
};

} // namespace pong::game
