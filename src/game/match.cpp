#include "match.hpp"

#include <gamelib/core/collision.hpp>

#include <algorithm>
#include <cmath>

namespace pong::game {

using gamelib::core::Rect;
using gamelib::core::Vec2;

namespace {

// Collider ids for moveAndCollide.
constexpr int kWallId = 0;
constexpr int kLeftPaddleId = 1;
constexpr int kRightPaddleId = 2;

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
    return Rect::fromCenter({x, y.value()}, kPaddleSize);
}

Match::Match(const Rules& rules, const std::uint64_t seed, const Side firstReceiver)
    : m_rules{rules}
    , m_rng{seed}
{
    startServe(firstReceiver);
}

std::vector<Event> Match::takeEvents()
{
    return m_events.take();
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
    m_ball.position.beginTick();
    for (Paddle& paddle : m_paddles) {
        paddle.y.beginTick();
    }
    movePaddles(input, dt);

    switch (m_phase) {
    case Phase::Serving:
        // Counted in whole ticks: exactly serveDelaySeconds at any tick rate
        // that divides it, with no float drift.
        if (!m_serveArmed) {
            m_serveCountdown.start(gamelib::core::Countdown::ticksFor(m_rules.serveDelaySeconds, dt));
            m_serveArmed = true;
        }
        if (m_serveCountdown.tick()) {
            serve();
        }
        break;
    case Phase::Rally: {
        moveBall(dt);
        // A point once the ball has completely left the court.
        const Vec2 ball = m_ball.position.value();
        if (ball.x + kBallRadius < 0.0f) {
            pointFor(Side::Right);
        } else if (ball.x - kBallRadius > kCourtWidth) {
            pointFor(Side::Left);
        }
        break;
    }
    case Phase::GameOver: break;
    }
}

void Match::movePaddles(const Input& input, const float dt)
{
    const std::array moves{std::clamp(input.left, -1.0f, 1.0f), std::clamp(input.right, -1.0f, 1.0f)};
    for (std::size_t i = 0; i < m_paddles.size(); ++i) {
        Paddle& paddle = m_paddles[i];
        float& y = paddle.y.value();
        const float before = y;
        y = std::clamp(y + moves[i] * m_rules.paddleSpeed * dt, kMinPaddleY, kMaxPaddleY);
        paddle.velocity = dt > 0.0f ? (y - before) / dt : 0.0f;
    }
}

void Match::moveBall(const float dt)
{
    const std::array<gamelib::core::Collider, 4> colliders{
        gamelib::core::Collider{kTopWall, kWallId},
        gamelib::core::Collider{kBottomWall, kWallId},
        gamelib::core::Collider{m_paddles[0].rect(Side::Left), kLeftPaddleId},
        gamelib::core::Collider{m_paddles[1].rect(Side::Right), kRightPaddleId},
    };
    const gamelib::core::MoveResult moved = gamelib::core::moveAndCollide(
        m_ball.position.value(), kBallRadius, m_ball.velocity, dt, colliders,
        [this](const gamelib::core::Contact& contact) {
            m_ball.position.value() = contact.hit.center; // bounceOffPaddle reads the contact point
            // A paddle's front face (or a corner mostly facing the court)
            // returns the ball with an angle; its ends and the walls reflect.
            if (contact.id != kWallId && std::abs(contact.hit.normal.x) > 0.5f) {
                return bounceOffPaddle(contact.id == kLeftPaddleId ? Side::Left : Side::Right, contact.hit.normal,
                                       contact.velocity);
            }
            const Vec2 velocity = gamelib::core::bounce(contact);
            m_events.push({.kind = contact.id == kWallId ? EventKind::WallHit : EventKind::PaddleHit,
                           .side = contact.id == kRightPaddleId ? Side::Right : Side::Left,
                           .speed = gamelib::core::length(velocity)});
            return velocity;
        });
    m_ball.position.value() = moved.center;
    m_ball.velocity = moved.velocity;
}

Vec2 Match::bounceOffPaddle(const Side side, const Vec2 normal, const Vec2 velocity)
{
    const Paddle& paddle = m_paddles[index(side)];
    // Where the ball met the paddle, -1 (top end) to +1 (bottom end): the
    // classic Pong control - the further from the center, the steeper.
    const float reach = kPaddleSize.y / 2.0f + kBallRadius;
    const float offset = std::clamp((m_ball.position.value().y - paddle.y.value()) / reach, -1.0f, 1.0f);
    const float angle = offset * _radians(m_rules.maxBounceDegrees);
    const float speed = std::min(gamelib::core::length(velocity) * m_rules.speedUpPerHit, m_rules.maxSpeed);

    const float away = normal.x > 0.0f ? 1.0f : -1.0f; // back towards the opponent
    Vec2 direction{away * std::cos(angle), std::sin(angle)};
    // A moving paddle drags the ball along a little ("spin").
    direction.y += m_rules.paddleSpin * paddle.velocity / std::max(speed, 1.0f);
    // Never let the drag make the return too flat to cross the court.
    direction.y = std::clamp(direction.y, -std::sin(_radians(m_rules.maxBounceDegrees)),
                             std::sin(_radians(m_rules.maxBounceDegrees)));

    ++m_rallyHits;
    m_events.push({.kind = EventKind::PaddleHit, .side = side, .speed = speed});
    return gamelib::core::normalized(direction) * speed;
}

void Match::pointFor(const Side scorer)
{
    ++m_score[index(scorer)];
    m_events.push({.kind = EventKind::Scored, .side = scorer});
    if (m_score[index(scorer)] >= m_rules.pointsToWin) {
        m_phase = Phase::GameOver;
        m_ball.position.snap({kCourtWidth / 2.0f, kCourtHeight / 2.0f});
        m_ball.velocity = {};
        m_events.push({.kind = EventKind::MatchWon, .side = scorer});
        return;
    }
    // The player who lost the point receives the next serve.
    startServe(opponent(scorer));
}

void Match::startServe(const Side receiver)
{
    m_phase = Phase::Serving;
    m_ball.position.snap({kCourtWidth / 2.0f, kCourtHeight / 2.0f}); // no streak across the court
    m_ball.velocity = {};
    m_serveArmed = false;
    m_receiver = receiver;
    m_rallyHits = 0;
}

void Match::serve()
{
    const float angle = _radians(m_rng.uniform(-m_rules.maxServeDegrees, m_rules.maxServeDegrees));
    const float towards = m_receiver == Side::Left ? -1.0f : 1.0f;
    m_ball.velocity = Vec2{towards * std::cos(angle), std::sin(angle)} * m_rules.serveSpeed;
    m_phase = Phase::Rally;
    m_events.push({.kind = EventKind::Served, .side = m_receiver, .speed = m_rules.serveSpeed});
}

void Match::placeBall(const Vec2 position, const Vec2 velocity)
{
    m_ball.position.snap(position);
    m_ball.velocity = velocity;
    m_phase = Phase::Rally;
}

void Match::placePaddle(const Side side, const float y)
{
    m_paddles[index(side)].y.snap(std::clamp(y, kMinPaddleY, kMaxPaddleY));
}

} // namespace pong::game
