#pragma once

#include <gamelib/audio/player.hpp>
#include <gamelib/audio/sound_queue.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace pong::app {

// Every sound effect in Pong, synthesized at startup with GameLib's chiptune
// synth.
enum class Sfx : std::uint8_t { PaddleHit, WallHit, Score, Serve, Win, MenuMove, MenuSelect, MenuBack };
inline constexpr std::size_t kSfxCount = 8;

// Renders one effect to mono float samples, exactly as the game plays it.
[[nodiscard]] std::vector<float> renderSfx(Sfx sfx, std::uint32_t sampleRate);

// The effects, rendered once and registered with the player; queue() adds
// them to the frame's sound queue (each plays once per frame).
class Sounds {
public:
    explicit Sounds(gamelib::audio::Player& player);

    void queue(gamelib::audio::SoundQueue& queue, Sfx sfx, float pitch = 1.0f) const;

private:
    std::array<gamelib::audio::SoundId, kSfxCount> m_ids{};
};

} // namespace pong::app
