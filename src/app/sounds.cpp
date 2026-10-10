#include "sounds.hpp"

#include <gamelib/audio/synth.hpp>

namespace pong::app {

namespace {

using gamelib::audio::Note;
using gamelib::audio::Wave;
using gamelib::audio::noise;
using gamelib::audio::tone;

// MIDI note numbers. Everything sits on an A minor pentatonic, so the
// sounds belong together.
constexpr float kA3 = 57, kC4 = 60, kE4 = 64, kA4 = 69, kC5 = 72, kD5 = 74, kE5 = 76, kG5 = 79, kA5 = 81;
constexpr float kC6 = 84, kE6 = 88, kA6 = 93;

struct Recipe {
    std::vector<std::vector<Note>> tracks;
    float peakDb = -14.0f; // loudest sample, dBFS
};

Recipe _recipe(const Sfx sfx)
{
    switch (sfx) {
    // The classic "pock": a short square blip with a click of noise.
    case Sfx::PaddleHit: return {{{tone(Wave::Pulse50, kA5, 0.05f)}, {noise(12000.0f, 0.012f, 0.0f, 0.5f)}}, -13.0f};
    case Sfx::WallHit: return {{{tone(Wave::Pulse25, kE5, 0.04f)}}, -17.0f};
    // A falling slide: someone missed.
    case Sfx::Score:
        return {{{tone(Wave::Pulse25, kE5, 0.09f, kA3), tone(Wave::Triangle, kA3, 0.16f, kA3 - 12.0f)},
                 {noise(4000.0f, 0.12f, 800.0f, 0.3f)}},
                -13.0f};
    case Sfx::Serve: return {{{tone(Wave::Pulse12, kA4, 0.03f), tone(Wave::Pulse12, kE5, 0.04f)}}, -19.0f};
    // A fanfare up the scale with the root underneath.
    case Sfx::Win:
        return {{{tone(Wave::Pulse25, kA4, 0.08f), tone(Wave::Pulse25, kC5, 0.08f), tone(Wave::Pulse25, kE5, 0.08f),
                  tone(Wave::Pulse25, kA5, 0.08f), tone(Wave::Pulse25, kE6, 0.08f), tone(Wave::Pulse25, kA6, 0.35f)},
                 {tone(Wave::Triangle, kA3, 0.4f), tone(Wave::Triangle, kE4, 0.35f)}},
                -12.0f};
    case Sfx::MenuMove: return {{{tone(Wave::Pulse12, kC6, 0.03f)}}, -18.0f};
    case Sfx::MenuSelect: return {{{tone(Wave::Pulse25, kG5, 0.04f), tone(Wave::Pulse25, kD5 + 12.0f, 0.07f)}}, -15.0f};
    case Sfx::MenuBack: return {{{tone(Wave::Pulse25, kG5, 0.04f), tone(Wave::Pulse25, kC4 + 12.0f, 0.07f)}}, -15.0f};
    }
    return {};
}

} // namespace

std::vector<float> renderSfx(const Sfx sfx, const std::uint32_t sampleRate)
{
    const Recipe recipe = _recipe(sfx);
    return gamelib::audio::renderMix(recipe.tracks, recipe.peakDb, sampleRate);
}

} // namespace pong::app
