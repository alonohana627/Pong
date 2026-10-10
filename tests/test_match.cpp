#include "match.hpp"

#include <gamelib/testing/test.hpp>

#include <algorithm>
#include <cmath>

using namespace pong::game;
using gamelib::core::Vec2;

namespace {

constexpr float kDt = 1.0f / 120.0f;

bool _has(const std::vector<Event>& events, const EventKind kind)
{
    return std::ranges::any_of(events, [&](const Event& e) { return e.kind == kind; });
}

// Ticks until the match leaves the Serving phase (at most 10 s).
std::vector<Event> _untilServed(Match& match)
{
    std::vector<Event> all;
    for (int i = 0; i < 1200 && match.phase() == Phase::Serving; ++i) {
        match.tick({}, kDt);
        const auto events = match.takeEvents();
        all.insert(all.end(), events.begin(), events.end());
    }
    return all;
}

} // namespace

TEST_CASE("a serve waits for the delay, then goes to the receiver within the angle range")
{
    for (std::uint64_t seed = 1; seed <= 50; ++seed) {
        Match match{Rules{}, seed, Side::Right};
        REQUIRE(match.phase() == Phase::Serving);
        CHECK_EQ(match.ball().position, (Vec2{kCourtWidth / 2.0f, kCourtHeight / 2.0f}));
        int ticks = 0;
        while (match.phase() == Phase::Serving && ticks < 1000) {
            match.tick({}, kDt);
            ++ticks;
        }
        CHECK_EQ(ticks, 120); // 1.0 s at 120 Hz
        const Vec2 v = match.ball().velocity;
        CHECK(v.x > 0.0f); // towards the right-hand receiver
        CHECK_NEAR(gamelib::core::length(v), Rules{}.serveSpeed, 1e-2);
        const float degrees = std::atan2(std::abs(v.y), v.x) * 180.0f / gamelib::core::kPi;
        CHECK(degrees <= Rules{}.maxServeDegrees + 1e-3f);
        const auto events = match.takeEvents();
        REQUIRE_EQ(events.size(), std::size_t{1});
        CHECK_EQ(events[0], (Event{.kind = EventKind::Served, .side = Side::Right, .speed = Rules{}.serveSpeed}));
    }
}

TEST_CASE("the ball bounces off the walls")
{
    Match match{Rules{}, 1};
    match.placeBall({960.0f, kWallThickness + kBallRadius + 5.0f}, {0.0f, -600.0f});
    match.tick({}, kDt); // 5 units of motion: hits the top wall
    CHECK(match.ball().velocity.y > 0.0f);
    CHECK_NEAR(match.ball().velocity.y, 600.0f, 1e-3);
    CHECK(match.ball().position.y >= kWallThickness + kBallRadius);
    CHECK(_has(match.takeEvents(), EventKind::WallHit));

    match.placeBall({960.0f, kCourtHeight - kWallThickness - kBallRadius - 1.0f}, {300.0f, 900.0f});
    match.tick({}, kDt);
    CHECK(match.ball().velocity.y < 0.0f);
    CHECK_NEAR(match.ball().velocity.x, 300.0f, 1e-3); // horizontal speed unchanged
}

TEST_CASE("a paddle returns the ball faster, angled by where it hit")
{
    const Rules rules;
    const float faceX = Paddle{}.rect(Side::Left).right();
    for (const float offset : {0.0f, -60.0f, 60.0f}) {
        Match match{rules, 1};
        match.placePaddle(Side::Left, 540.0f);
        match.placeBall({faceX + kBallRadius + 4.0f, 540.0f + offset}, {-800.0f, 0.0f});
        match.tick({}, kDt);
        const Vec2 v = match.ball().velocity;
        CHECK(v.x > 0.0f); // back towards the right
        CHECK_NEAR(gamelib::core::length(v), 800.0f * rules.speedUpPerHit, 0.05);
        if (offset == 0.0f) {
            CHECK_NEAR(v.y, 0.0f, 1e-3);
        } else {
            CHECK((offset < 0.0f) == (v.y < 0.0f)); // above the center: goes up
        }
        const auto events = match.takeEvents();
        REQUIRE(_has(events, EventKind::PaddleHit));
        CHECK_EQ(match.rallyHits(), 1);
    }
}

TEST_CASE("the bounce angle never exceeds the maximum and speed is capped")
{
    Rules rules;
    rules.maxSpeed = 1000.0f;
    Match match{rules, 1};
    const float faceX = Paddle{}.rect(Side::Right).left();
    match.placePaddle(Side::Right, 540.0f);
    // Hit with the very end of the paddle at a speed above the cap.
    match.placeBall({faceX - kBallRadius - 2.0f, 540.0f + 90.0f}, {1500.0f, 0.0f});
    match.tick({.right = 1.0f}, kDt); // a moving paddle adds spin
    const Vec2 v = match.ball().velocity;
    CHECK(v.x < 0.0f);
    CHECK_NEAR(gamelib::core::length(v), 1000.0f, 0.05);
    const float degrees = std::atan2(std::abs(v.y), std::abs(v.x)) * 180.0f / gamelib::core::kPi;
    CHECK(degrees <= rules.maxBounceDegrees + 0.01f);
}

TEST_CASE("a fast ball cannot tunnel through a paddle")
{
    Rules rules;
    rules.maxSpeed = 100'000.0f;
    Match match{rules, 1};
    match.placePaddle(Side::Left, 540.0f);
    // 60'000 units/s = 500 units per tick: from x = 400 past the paddle at
    // x ~ 90 within one tick.
    match.placeBall({400.0f, 540.0f}, {-60'000.0f, 0.0f});
    match.tick({}, kDt);
    CHECK(match.ball().velocity.x > 0.0f);
    CHECK(match.ball().position.x > Paddle{}.rect(Side::Left).right());
    CHECK_EQ(match.score(Side::Right), 0);
}

TEST_CASE("a missed ball scores for the other side and the loser receives next")
{
    Match match{Rules{}, 3};
    match.placePaddle(Side::Left, kCourtHeight - 200.0f); // far from the ball
    match.placeBall({60.0f, 300.0f}, {-1000.0f, 0.0f});
    for (int i = 0; i < 20 && match.phase() == Phase::Rally; ++i) {
        match.tick({}, kDt);
    }
    CHECK_EQ(match.score(Side::Right), 1);
    CHECK_EQ(match.score(Side::Left), 0);
    CHECK(match.phase() == Phase::Serving);
    CHECK(match.receiver() == Side::Left);
    CHECK_EQ(match.ball().position, (Vec2{kCourtWidth / 2.0f, kCourtHeight / 2.0f}));
    CHECK_EQ(match.previousBallPosition(), match.ball().position); // no streak across the court
    const auto events = match.takeEvents();
    REQUIRE(_has(events, EventKind::Scored));
    CHECK(std::ranges::find(events, Event{.kind = EventKind::Scored, .side = Side::Right}) != events.end());

    const auto served = _untilServed(match);
    CHECK(match.ball().velocity.x < 0.0f); // served towards the left player
    CHECK(_has(served, EventKind::Served));
}

TEST_CASE("the match ends at the winning score and then stays over")
{
    Rules rules;
    rules.pointsToWin = 3;
    Match match{rules, 5};
    for (int point = 0; point < 3; ++point) {
        _untilServed(match);
        match.placePaddle(Side::Right, kWallThickness); // out of the way
        match.placeBall({kCourtWidth - 40.0f, 900.0f}, {1500.0f, 0.0f});
        for (int i = 0; i < 20 && match.phase() == Phase::Rally; ++i) {
            match.tick({}, kDt);
        }
    }
    CHECK(match.phase() == Phase::GameOver);
    CHECK_EQ(match.score(Side::Left), 3);
    REQUIRE(match.winner().has_value());
    CHECK(*match.winner() == Side::Left);
    CHECK(_has(match.takeEvents(), EventKind::MatchWon));

    for (int i = 0; i < 600; ++i) {
        match.tick({}, kDt);
    }
    CHECK(match.phase() == Phase::GameOver);
    CHECK_EQ(match.score(Side::Left), 3);
    CHECK(match.takeEvents().empty());
}

TEST_CASE("paddles move with input and stay inside the court")
{
    Match match{Rules{}, 1};
    const float start = match.paddle(Side::Left).y;
    match.tick({.left = -1.0f, .right = 0.5f}, kDt);
    CHECK_NEAR(match.paddle(Side::Left).y, start - Rules{}.paddleSpeed * kDt, 1e-3);
    CHECK_NEAR(match.paddle(Side::Right).y, start + 0.5f * Rules{}.paddleSpeed * kDt, 1e-3);
    CHECK_NEAR(match.paddle(Side::Left).velocity, -Rules{}.paddleSpeed, 1e-2);
    CHECK_NEAR(match.previousPaddleY(Side::Left), start, 0.0);

    for (int i = 0; i < 400; ++i) {
        match.tick({.left = -5.0f, .right = 1.0f}, kDt); // out-of-range input is clamped
    }
    CHECK_NEAR(match.paddle(Side::Left).rect(Side::Left).top(), kWallThickness, 1e-3);
    CHECK_NEAR(match.paddle(Side::Right).rect(Side::Right).bottom(), kCourtHeight - kWallThickness, 1e-3);
    CHECK_NEAR(match.paddle(Side::Left).velocity, 0.0f, 1e-3); // stopped at the wall
}

TEST_CASE("the simulation is deterministic for a seed and inputs")
{
    const auto play = [](const std::uint64_t seed) {
        Match match{Rules{}, seed};
        for (int i = 0; i < 5000; ++i) {
            const float wave = std::sin(static_cast<float>(i) * 0.05f);
            match.tick({.left = wave, .right = -wave}, kDt);
        }
        return std::tuple{match.ball().position.x, match.ball().position.y, match.score(Side::Left),
                          match.score(Side::Right)};
    };
    CHECK(play(11) == play(11));
    CHECK(play(11) != play(12));
}
