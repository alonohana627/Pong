#include "pong_game.hpp"
#include "settings.hpp"

#include <gamelib/app/app.hpp>

#include <memory>

int main(int argc, char** argv)
{
    // Loaded before the window exists, so a saved fullscreen setting is
    // applied while the window is still hidden.
    const std::filesystem::path path = pong::app::settingsPath();
    const pong::app::Settings settings = pong::app::loadSettings(path);

    const gamelib::app::AppConfig config{
        .title = "Pong",
        .windowSize = {1280, 720},
        .canvas = {1920, 1080},
        .tick = std::chrono::nanoseconds{1'000'000'000 / 120},
        .fullscreen = settings.fullscreen,
        .f11TogglesFullscreen = false, // Pong handles F11 itself, to save the setting
        .gameFlags = {"--demo"},
    };
    return gamelib::app::runMain(argc, argv, config, [&](gamelib::app::App& app) {
        return std::make_unique<pong::app::PongGame>(app, settings, path, app.hasFlag("--demo"));
    });
}
