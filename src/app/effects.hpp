#pragma once

#include "match.hpp"

#include <gamelib/fx/juice.hpp>

#include <array>

namespace pong::app {

// Pong's visual feedback, assembled from GameLib's fx parts: a paddle flashes
// when it hits the ball, the screen shakes and the scorer's digits pulse on a
// point, and the ball leaves a short trail. Presentation only: it runs on real
// frame time and never touches the match.
struct Effects {
    static constexpr std::size_t kTrailLength = 10;

    std::array<gamelib::fx::Fade, 2> paddleFlash{gamelib::fx::Fade{0.25f}, gamelib::fx::Fade{0.25f}};
    std::array<gamelib::fx::Fade, 2> scorePulse{gamelib::fx::Fade{0.6f}, gamelib::fx::Fade{0.6f}};
    gamelib::fx::Shake shake{0.35f, 14.0f};
    gamelib::fx::Trail<kTrailLength> trail;

    void onPaddleHit(game::Side side) { paddleFlash[game::index(side)].trigger(); }
    void onScore(const game::Side scorer)
    {
        scorePulse[game::index(scorer)].trigger();
        shake.trigger();
    }
    void update(const float seconds)
    {
        for (gamelib::fx::Fade& fade : paddleFlash) {
            fade.update(seconds);
        }
        for (gamelib::fx::Fade& fade : scorePulse) {
            fade.update(seconds);
        }
        shake.update(seconds);
    }
    void clear() { *this = Effects{}; }
};

} // namespace pong::app
