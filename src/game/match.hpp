#pragma once

#include <gamelib/core/math.hpp>
#include <gamelib/core/rng.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace pong::game {

// The rules of Pong: two paddles, one ball, walls at the top and bottom, a
// point when the ball leaves the court on the left or right. Pure simulation:
// no window, keys, pixels, sounds or clock - the caller advances it in fixed
// ticks with the paddles' inputs, so it is fully testable and deterministic
// (same seed + same inputs = same game).
//
// Units are canvas units: the court is the 1920x1080 canvas, y grows downwards.

inline constexpr float kCourtWidth = 1920.0f;
inline constexpr float kCourtHeight = 1080.0f;
inline constexpr float kWallThickness = 24.0f;    // top and bottom walls, inside the court
inline constexpr float kPaddleInset = 90.0f;      // paddle center distance from the court edge
inline constexpr gamelib::core::Vec2 kPaddleSize{26.0f, 168.0f};
inline constexpr float kBallRadius = 15.0f;

enum class Side : std::uint8_t { Left, Right };

[[nodiscard]] constexpr Side opponent(const Side side) { return side == Side::Left ? Side::Right : Side::Left; }
[[nodiscard]] constexpr std::size_t index(const Side side) { return side == Side::Left ? 0 : 1; }

struct Rules {
    int pointsToWin = 11;
    float paddleSpeed = 1150.0f;       // units per second at full input
    float serveSpeed = 760.0f;         // ball speed after a serve
    float speedUpPerHit = 1.06f;       // each paddle return multiplies the speed
    float maxSpeed = 1900.0f;
    float maxBounceDegrees = 55.0f;    // bounce angle at the paddle's very end
    float maxServeDegrees = 28.0f;     // serves leave within +-this angle
    float serveDelaySeconds = 1.0f;    // pause before each serve
    float paddleSpin = 0.12f;          // fraction of the paddle's speed given to the ball
};

struct Paddle {
    float y = kCourtHeight / 2.0f; // center
    float velocity = 0.0f;         // units per second during the last tick

    [[nodiscard]] gamelib::core::Rect rect(Side side) const;
};

struct Ball {
    gamelib::core::Vec2 position{kCourtWidth / 2.0f, kCourtHeight / 2.0f};
    gamelib::core::Vec2 velocity;
};

enum class Phase : std::uint8_t {
    Serving,  // the ball waits in the center for the serve delay
    Rally,    // the ball is in play
    GameOver, // someone reached Rules::pointsToWin
};

enum class EventKind : std::uint8_t {
    Served,    // side = who receives
    PaddleHit, // side = whose paddle; speed = ball speed after the hit
    WallHit,
    Scored,    // side = who scored
    MatchWon,  // side = the winner
};

struct Event {
    EventKind kind = EventKind::Served;
    Side side = Side::Left;
    float speed = 0.0f;

    friend constexpr bool operator==(const Event&, const Event&) = default;
};

// What each paddle wants this tick: -1 = full speed up, +1 = full speed down,
// anything between for slower movement (the computer player uses that).
struct Input {
    float left = 0.0f;
    float right = 0.0f;
};

class Match {
public:
    // firstReceiver: who the first serve goes to.
    Match(const Rules& rules, std::uint64_t seed, Side firstReceiver = Side::Left);

    // Advances the match by one fixed tick of `dt` seconds.
    void tick(const Input& input, float dt);

    // The events since the last call, oldest first.
    [[nodiscard]] std::vector<Event> takeEvents();

    [[nodiscard]] const Rules& rules() const { return m_rules; }
    [[nodiscard]] Phase phase() const { return m_phase; }
    [[nodiscard]] int score(Side side) const { return m_score[index(side)]; }
    [[nodiscard]] std::optional<Side> winner() const;
    [[nodiscard]] const Ball& ball() const { return m_ball; }
    [[nodiscard]] const Paddle& paddle(Side side) const { return m_paddles[index(side)]; }
    // Where the ball and paddles were before the last tick: views interpolate
    // between these and the current state for smooth motion at any frame rate.
    [[nodiscard]] gamelib::core::Vec2 previousBallPosition() const { return m_previousBall; }
    [[nodiscard]] float previousPaddleY(Side side) const { return m_previousPaddleY[index(side)]; }
    // Seconds left before the serve (Serving phase only).
    [[nodiscard]] float serveCountdown() const { return m_serveCountdown; }
    // The side the next (or last) serve went to.
    [[nodiscard]] Side receiver() const { return m_receiver; }
    // Paddle returns in the current rally.
    [[nodiscard]] int rallyHits() const { return m_rallyHits; }

    // For tests and replays: put the ball somewhere specific, in play.
    void placeBall(gamelib::core::Vec2 position, gamelib::core::Vec2 velocity);
    void placePaddle(Side side, float y);

private:
    void movePaddles(const Input& input, float dt);
    void moveBall(float dt);
    void bounceOffPaddle(Side side, gamelib::core::Vec2 normal);
    void pointFor(Side scorer);
    void startServe(Side receiver);
    void serve();

    Rules m_rules;
    gamelib::core::Rng m_rng;
    Phase m_phase = Phase::Serving;
    std::array<int, 2> m_score{};
    std::array<Paddle, 2> m_paddles{};
    std::array<float, 2> m_previousPaddleY{kCourtHeight / 2.0f, kCourtHeight / 2.0f};
    Ball m_ball;
    gamelib::core::Vec2 m_previousBall = m_ball.position;
    float m_serveCountdown = 0.0f;
    Side m_receiver = Side::Left;
    int m_rallyHits = 0;
    std::vector<Event> m_events;
};

} // namespace pong::game
