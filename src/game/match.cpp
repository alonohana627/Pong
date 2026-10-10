#include "match.hpp"

#include <gamelib/core/collision.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace pong::game {

using gamelib::core::Rect;
using gamelib::core::Vec2;

namespace {

// Collisions can chain within one tick (wall, then paddle). More than this
// many in a single tick only happens in degenerate corners; the rest of the
// motion is then dropped for that tick.
constexpr int kMaxBouncesPerTick = 4;
// After a contact the ball is pushed this far off the surface, so the next
// sweep does not report the same contact again.
constexpr float kSeparation = 0.01f;

// The walls extend far beyond the court sideways and outwards, so a ball can
// never slip around their ends.
constexpr Rect kTopWall{-10'000.0f, -10'000.0f, kCourtWidth + 20'000.0f, 10'000.0f + kWallThickness};
constexpr Rect kBottomWall{-10'000.0f, kCourtHeight - kWallThickness, kCourtWidth + 20'000.0f, 10'000.0f};

constexpr float kMinPaddleY = kWallThickness + kPaddleSize.y / 2.0f;
constexpr float kMaxPaddleY = kCourtHeight - kWallThickness - kPaddleSize.y / 2.0f;

float _radians(const float degrees)
{
    return degrees * gamelib::core::kPi / 180.0f;
}

} // namespace

Rect Paddle::rect(const Side side) const
{
    const float x = side == Side::Left ? kPaddleInset : kCourtWidth - kPaddleInset;
    return Rect::fromCenter({x, y}, kPaddleSize);
}

Match::Match(const Rules& rules, const std::uint64_t seed, const Side firstReceiver)
    : m_rules{rules}
    , m_rng{seed}
{
    startServe(firstReceiver);
}

std::vector<Event> Match::takeEvents()
{
    return std::exchange(m_events, {});
}

std::optional<Side> Match::winner() const
{
    if (m_phase != Phase::GameOver) {
        return std::nullopt;
    }
    return score(Side::Left) > score(Side::Right) ? Side::Left : Side::Right;
}

void Match::tick(const Input& input, const float dt)
{
    m_previousBall = m_ball.position;
    for (const Side side : {Side::Left, Side::Right}) {
        m_previousPaddleY[index(side)] = m_paddles[index(side)].y;
    }
    movePaddles(input, dt);

    switch (m_phase) {
    case Phase::Serving:
        m_serveCountdown -= dt;
        // A small tolerance: 120 ticks of 1/120 s add up to slightly less than
        // 1 s in floating point, which would delay the serve by a tick.
        if (m_serveCountdown <= 1e-4f) {
            serve();
        }
        break;
    case Phase::Rally:
        moveBall(dt);
        // A point once the ball has completely left the court.
        if (m_ball.position.x + kBallRadius < 0.0f) {
            pointFor(Side::Right);
        } else if (m_ball.position.x - kBallRadius > kCourtWidth) {
            pointFor(Side::Left);
        }
        break;
    case Phase::GameOver: break;
    }
}

void Match::movePaddles(const Input& input, const float dt)
{
    const std::array moves{std::clamp(input.left, -1.0f, 1.0f), std::clamp(input.right, -1.0f, 1.0f)};
    for (std::size_t i = 0; i < m_paddles.size(); ++i) {
        Paddle& paddle = m_paddles[i];
        const float before = paddle.y;
        paddle.y = std::clamp(paddle.y + moves[i] * m_rules.paddleSpeed * dt, kMinPaddleY, kMaxPaddleY);
        paddle.velocity = dt > 0.0f ? (paddle.y - before) / dt : 0.0f;
    }
}

void Match::moveBall(const float dt)
{
    float remaining = 1.0f; // fraction of this tick's motion still to do
    for (int bounce = 0; bounce < kMaxBouncesPerTick && remaining > 0.0f; ++bounce) {
        const Vec2 motion = m_ball.velocity * (dt * remaining);

        // The earliest contact among the walls and paddles along the motion.
        struct Obstacle {
            Rect rect;
            std::optional<Side> paddle; // nullopt: a wall
        };
        const std::array<Obstacle, 4> obstacles{
            Obstacle{kTopWall, std::nullopt},
            Obstacle{kBottomWall, std::nullopt},
            Obstacle{m_paddles[0].rect(Side::Left), Side::Left},
            Obstacle{m_paddles[1].rect(Side::Right), Side::Right},
        };
        std::optional<gamelib::core::SweepHit> first;
        std::optional<Side> firstPaddle;
        for (const Obstacle& obstacle : obstacles) {
            const auto hit = gamelib::core::sweep(m_ball.position, kBallRadius, motion, obstacle.rect);
            if (hit && (!first || hit->time < first->time)) {
                first = hit;
                firstPaddle = obstacle.paddle;
            }
        }
        if (!first) {
            m_ball.position += motion;
            return;
        }

        m_ball.position = first->center + first->normal * kSeparation;
        remaining *= 1.0f - first->time;
        // A paddle's front face (or a corner mostly facing the court) returns
        // the ball with an angle; its top and bottom ends just deflect it.
        if (firstPaddle && std::abs(first->normal.x) > 0.5f) {
            bounceOffPaddle(*firstPaddle, first->normal);
        } else {
            m_ball.velocity = gamelib::core::reflect(m_ball.velocity, first->normal);
            m_events.push_back({.kind = firstPaddle ? EventKind::PaddleHit : EventKind::WallHit,
                                .side = firstPaddle.value_or(Side::Left),
                                .speed = gamelib::core::length(m_ball.velocity)});
        }
    }
}

void Match::bounceOffPaddle(const Side side, const Vec2 normal)
{
    const Paddle& paddle = m_paddles[index(side)];
    // Where the ball met the paddle, -1 (top end) to +1 (bottom end): the
    // classic Pong control - the further from the center, the steeper.
    const float reach = kPaddleSize.y / 2.0f + kBallRadius;
    const float offset = std::clamp((m_ball.position.y - paddle.y) / reach, -1.0f, 1.0f);
    const float angle = offset * _radians(m_rules.maxBounceDegrees);
    const float speed = std::min(gamelib::core::length(m_ball.velocity) * m_rules.speedUpPerHit, m_rules.maxSpeed);

    const float away = normal.x > 0.0f ? 1.0f : -1.0f; // back towards the opponent
    Vec2 direction{away * std::cos(angle), std::sin(angle)};
    // A moving paddle drags the ball along a little ("spin").
    direction.y += m_rules.paddleSpin * paddle.velocity / std::max(speed, 1.0f);
    // Never let the drag make the return too flat to cross the court.
    direction.y = std::clamp(direction.y, -std::sin(_radians(m_rules.maxBounceDegrees)),
                             std::sin(_radians(m_rules.maxBounceDegrees)));
    m_ball.velocity = gamelib::core::normalized(direction) * speed;

    ++m_rallyHits;
    m_events.push_back({.kind = EventKind::PaddleHit, .side = side, .speed = speed});
}

void Match::pointFor(const Side scorer)
{
    ++m_score[index(scorer)];
    m_events.push_back({.kind = EventKind::Scored, .side = scorer});
    if (m_score[index(scorer)] >= m_rules.pointsToWin) {
        m_phase = Phase::GameOver;
        m_ball = Ball{};
        m_previousBall = m_ball.position;
        m_events.push_back({.kind = EventKind::MatchWon, .side = scorer});
        return;
    }
    // The player who lost the point receives the next serve.
    startServe(opponent(scorer));
}

void Match::startServe(const Side receiver)
{
    m_phase = Phase::Serving;
    m_ball = Ball{};
    m_previousBall = m_ball.position; // no interpolation streak across the court
    m_serveCountdown = m_rules.serveDelaySeconds;
    m_receiver = receiver;
    m_rallyHits = 0;
}

void Match::serve()
{
    const float angle = _radians(m_rng.uniform(-m_rules.maxServeDegrees, m_rules.maxServeDegrees));
    const float towards = m_receiver == Side::Left ? -1.0f : 1.0f;
    m_ball.velocity = Vec2{towards * std::cos(angle), std::sin(angle)} * m_rules.serveSpeed;
    m_phase = Phase::Rally;
    m_events.push_back({.kind = EventKind::Served, .side = m_receiver, .speed = m_rules.serveSpeed});
}

void Match::placeBall(const Vec2 position, const Vec2 velocity)
{
    m_ball = {.position = position, .velocity = velocity};
    m_previousBall = position;
    m_phase = Phase::Rally;
}

void Match::placePaddle(const Side side, const float y)
{
    m_paddles[index(side)].y = std::clamp(y, kMinPaddleY, kMaxPaddleY);
    m_previousPaddleY[index(side)] = m_paddles[index(side)].y;
}

} // namespace pong::game
