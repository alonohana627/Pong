#include "ai.hpp"

#include <gamelib/testing/test.hpp>

#include <cmath>

using namespace pong::game;
using gamelib::core::Vec2;

namespace {

constexpr float kDt = 1.0f / 120.0f;
constexpr float kTop = kWallThickness + kBallRadius;
constexpr float kBottom = kCourtHeight - kWallThickness - kBallRadius;

} // namespace

TEST_CASE("predictY follows straight lines and folds wall bounces")
{
    // Straight: no wall in the way.
    CHECK_NEAR(predictY({960.0f, 540.0f}, {-500.0f, 100.0f}, 460.0f), 640.0f, 1e-3);
    // Exactly one bounce off the bottom: would reach kBottom + 100 unfolded.
    const float t = 1.0f;
    const Vec2 start{960.0f, kBottom - 200.0f};
    const Vec2 velocity{-400.0f, 300.0f}; // after 1 s: 300 down = 100 past the line
    CHECK_NEAR(predictY(start, velocity, start.x + velocity.x * t), kBottom - 100.0f, 1e-2);
    // Many bounces stay inside the court.
    const float many = predictY({1800.0f, 300.0f}, {-200.0f, 3000.0f}, 100.0f);
    CHECK(many >= kTop && many <= kBottom);
    // Moving away or not moving: the current height.
    CHECK_NEAR(predictY({500.0f, 321.0f}, {0.0f, 50.0f}, 100.0f), 321.0f, 0.0);
}

TEST_CASE("the AI heads for the predicted intercept and waits in the middle")
{
    Match match{Rules{}, 1};
    Ai ai{Side::Right, Difficulty::Hard, 7};
    match.placeBall({960.0f, 540.0f}, {700.0f, -300.0f});
    const float input = ai.decide(match);
    const float paddleX = match.paddle(Side::Right).rect(Side::Right).center().x;
    const float expected = predictY(match.ball().position.value(), match.ball().velocity, paddleX);
    // The aim error is bounded by the tuning: within a paddle-and-a-ball of the intercept.
    CHECK(std::abs(ai.target() - expected) <= kPaddleSize.y / 2.0f + kBallRadius);
    CHECK((input < 0.0f) == (ai.target() < match.paddle(Side::Right).y.value()));

    // Ball moving away: back to the center after the next re-plan.
    match.placeBall({960.0f, 540.0f}, {-700.0f, 0.0f});
    for (int i = 0; i < 20; ++i) {
        static_cast<void>(ai.decide(match));
    }
    CHECK_NEAR(ai.target(), kCourtHeight / 2.0f, 0.0);
}

TEST_CASE("easier AIs react slower and move slower")
{
    Match match{Rules{}, 1};
    match.placePaddle(Side::Left, 200.0f);
    match.placeBall({960.0f, 900.0f}, {-700.0f, 0.0f}); // far below the paddle
    Ai easy{Side::Left, Difficulty::Easy, 1};
    Ai hard{Side::Left, Difficulty::Hard, 1};
    const float easyInput = easy.decide(match);
    const float hardInput = hard.decide(match);
    CHECK(easyInput > 0.0f && hardInput > 0.0f);
    CHECK(easyInput < hardInput);
    CHECK(hardInput <= 1.0f);
}

TEST_CASE("AI against AI finishes matches, and Hard beats Easy")
{
    int hardWins = 0;
    for (std::uint64_t seed = 1; seed <= 6; ++seed) {
        Rules rules;
        rules.pointsToWin = 5;
        Match match{rules, seed};
        Ai left{Side::Left, Difficulty::Hard, seed * 31};
        Ai right{Side::Right, Difficulty::Easy, seed * 57};
        int ticks = 0;
        // 10 minutes of game time is far more than a 5-point match needs.
        while (match.phase() != Phase::GameOver && ticks < 120 * 600) {
            match.tick({.left = left.decide(match), .right = right.decide(match)}, kDt);
            static_cast<void>(match.takeEvents());
            ++ticks;
        }
        REQUIRE(match.phase() == Phase::GameOver);
        hardWins += match.winner() == Side::Left ? 1 : 0;
    }
    CHECK(hardWins >= 5); // Hard should win (nearly) every match against Easy
}

TEST_CASE("even Hard misses sometimes: rallies do not go on forever")
{
    Rules rules;
    rules.pointsToWin = 3;
    Match match{rules, 99};
    Ai left{Side::Left, Difficulty::Hard, 1};
    Ai right{Side::Right, Difficulty::Hard, 2};
    int ticks = 0;
    while (match.phase() != Phase::GameOver && ticks < 120 * 900) {
        match.tick({.left = left.decide(match), .right = right.decide(match)}, kDt);
        ++ticks;
    }
    CHECK(match.phase() == Phase::GameOver);
    CHECK_EQ(name(Difficulty::Hard), std::string_view{"HARD"});
}
