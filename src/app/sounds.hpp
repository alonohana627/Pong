#pragma once

#include <gamelib/audio/sound_bank.hpp>

#include <cstdint>
#include <vector>

namespace pong::app {

// Every sound effect in Pong, synthesized at startup with GameLib's chiptune
// synth.
enum class Sfx : std::uint8_t { PaddleHit, WallHit, Score, Serve, Win, MenuMove, MenuSelect, MenuBack };
inline constexpr std::size_t kSfxCount = 8;

// Renders one effect to mono float samples, exactly as the game plays it.
[[nodiscard]] std::vector<float> renderSfx(Sfx sfx, std::uint32_t sampleRate);

// The effects, rendered once and registered with the player by GameLib's
// SoundBank, then queued by Sfx (each plays once per frame).
using Sounds = gamelib::audio::SoundBank<Sfx, kSfxCount>;

} // namespace pong::app
