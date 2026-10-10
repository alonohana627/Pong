#include "settings.hpp"

#include <gamelib/storage/files.hpp>
#include <gamelib/storage/ini.hpp>

#include <cstdio>
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

Settings loadSettings(const std::filesystem::path& path)
{
    using namespace gamelib::storage;
    Settings settings; // defaults; the file only overrides
    const std::optional<std::string> text = readFile(path);
    if (!text) {
        return settings;
    }
    const IniFile ini = IniFile::parse(*text);
    if (const auto value = ini.get("settings", "difficulty")) {
        if (const auto choice = parseChoice(*value, {"easy", "normal", "hard"})) {
            settings.difficulty = static_cast<game::Difficulty>(*choice);
        }
    }
    if (const auto value = ini.get("settings", "sound")) {
        if (const auto choice = parseChoice(*value, {"off", "low", "medium", "high"})) {
            settings.volume = static_cast<gamelib::audio::VolumeLevel>(*choice);
        }
    }
    if (const auto value = ini.get("settings", "fullscreen")) {
        settings.fullscreen = parseBool(*value).value_or(settings.fullscreen);
    }
    return settings;
}

bool saveSettings(const std::filesystem::path& path, const Settings& settings)
{
    using namespace gamelib::storage;
    IniFile ini;
    ini.setHeaderComment("Pong settings. difficulty: easy/normal/hard, sound: off/low/medium/high, "
                         "fullscreen: true/false.");
    ini.set("", "version", std::to_string(kFormatVersion));
    ini.set("settings", "difficulty", toLower(game::name(settings.difficulty)));
    ini.set("settings", "sound", toLower(gamelib::audio::name(settings.volume)));
    ini.set("settings", "fullscreen", settings.fullscreen ? "true" : "false");
    if (const std::error_code error = writeFileAtomic(path, ini.serialize())) {
        std::fprintf(stderr, "warning: could not save %s: %s\n", displayPath(path).c_str(), error.message().c_str());
        return false;
    }
    return true;
}

} // namespace pong::app
