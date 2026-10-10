#pragma once

#include "ai.hpp"

#include <gamelib/audio/player.hpp>
#include <gamelib/core/options.hpp>
#include <gamelib/storage/persisted.hpp>

#include <filesystem>

namespace pong::app {

// What the player chooses in the menu, kept between runs in pong.ini.
struct Settings {
    game::Difficulty difficulty = game::Difficulty::Normal;
    gamelib::audio::VolumeLevel volume = gamelib::audio::VolumeLevel::Medium;
    bool fullscreen = false;

    friend bool operator==(const Settings&, const Settings&) = default;
};

inline constexpr gamelib::core::OptionSet<gamelib::audio::VolumeLevel, 4> kVolumes{{"OFF", "LOW", "MEDIUM", "HIGH"}};

// The settings file, loaded with defaults for anything missing or broken and
// written only when something changed (GameLib's storage::Persisted).
using SettingsFile = gamelib::storage::Persisted<Settings>;

// pong.ini in the user data folder (%APPDATA%\Pong on Windows). The
// environment variable PONG_DATA_DIR overrides the folder (tests use it).
[[nodiscard]] std::filesystem::path settingsPath();

[[nodiscard]] SettingsFile openSettings(const std::filesystem::path& path);

} // namespace pong::app
