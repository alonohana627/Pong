#pragma once

#include "ai.hpp"

#include <gamelib/audio/player.hpp>

#include <filesystem>

namespace pong::app {

// What the player chooses in the menu, kept between runs in pong.ini.
struct Settings {
    game::Difficulty difficulty = game::Difficulty::Normal;
    gamelib::audio::VolumeLevel volume = gamelib::audio::VolumeLevel::Medium;
    bool fullscreen = false;

    friend bool operator==(const Settings&, const Settings&) = default;
};

// pong.ini in the user data folder (%APPDATA%\Pong on Windows). The
// environment variable PONG_DATA_DIR overrides the folder (tests use it).
[[nodiscard]] std::filesystem::path settingsPath();

// Never fails: a missing file, key or unreadable value keeps its default.
[[nodiscard]] Settings loadSettings(const std::filesystem::path& path);
// Writes atomically; returns false (after a warning) if it could not.
bool saveSettings(const std::filesystem::path& path, const Settings& settings);

} // namespace pong::app
