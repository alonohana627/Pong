#include "settings.hpp"

#include <gamelib/storage/files.hpp>
#include <gamelib/storage/ini.hpp>

#include <string>

namespace pong::app {

namespace {

constexpr const char* kFileName = "pong.ini";
constexpr int kFormatVersion = 1;

} // namespace

std::filesystem::path settingsPath()
{
    return gamelib::storage::userDataDir("Pong", "PONG_DATA_DIR") / kFileName;
}

SettingsFile openSettings(const std::filesystem::path& path)
{
    using gamelib::storage::IniFile;
    return SettingsFile{
        path,
        [](const IniFile& ini) {
            Settings settings; // defaults; the file only overrides
            if (const auto value = ini.get("settings", "difficulty")) {
                settings.difficulty = game::kDifficulties.parse(*value).value_or(settings.difficulty);
            }
            if (const auto value = ini.get("settings", "sound")) {
                settings.volume = kVolumes.parse(*value).value_or(settings.volume);
            }
            if (const auto value = ini.get("settings", "fullscreen")) {
                settings.fullscreen = gamelib::storage::parseBool(*value).value_or(settings.fullscreen);
            }
            return settings;
        },
        [](const Settings& settings, IniFile& ini) {
            using gamelib::storage::toLower;
            ini.set("", "version", std::to_string(kFormatVersion));
            ini.set("settings", "difficulty", toLower(game::kDifficulties.name(settings.difficulty)));
            ini.set("settings", "sound", toLower(kVolumes.name(settings.volume)));
            ini.set("settings", "fullscreen", settings.fullscreen ? "true" : "false");
        },
        "Pong settings. difficulty: easy/normal/hard, sound: off/low/medium/high, fullscreen: true/false."};
}

} // namespace pong::app
