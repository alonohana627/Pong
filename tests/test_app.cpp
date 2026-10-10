// Headless tests of Pong's presentation logic: its menu pages, the settings
// file, the sound recipes and the effects. No window, GPU or audio device.
// (The menu, settings-file, sound-bank and effect mechanics themselves are
// GameLib's and tested there.)
#include "effects.hpp"
#include "menus.hpp"
#include "settings.hpp"
#include "sounds.hpp"

#include <gamelib/storage/files.hpp>
#include <gamelib/testing/helpers.hpp>
#include <gamelib/testing/test.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>

using namespace pong;
using namespace pong::app;
using gamelib::testing::TempDir;
using gamelib::ui::MenuInput;
namespace fs = std::filesystem;

TEST_CASE("title menu: games, settings and quit in Pong's order")
{
    Settings settings;
    Menus menus = buildMenus(settings);
    gamelib::ui::Menu& title = menus.title;
    REQUIRE_EQ(title.items().size(), std::size_t{6});
    CHECK_EQ(title.items()[0].label, std::string{"1 PLAYER"});
    CHECK_EQ(title.handle(MenuInput::Confirm).command, static_cast<int>(StartOnePlayer));
    static_cast<void>(title.handle(MenuInput::Down));
    CHECK_EQ(title.handle(MenuInput::Confirm).command, static_cast<int>(StartTwoPlayers));
    static_cast<void>(title.handle(MenuInput::Up));
    static_cast<void>(title.handle(MenuInput::Up)); // wraps to QUIT
    CHECK_EQ(title.handle(MenuInput::Confirm).command, static_cast<int>(Quit));
    CHECK_EQ(title.handle(MenuInput::Back).command, gamelib::ui::kNoCommand); // nothing to go back to
}

TEST_CASE("title menu settings edit Pong's settings in place")
{
    Settings settings;
    Menus menus = buildMenus(settings);
    gamelib::ui::Menu& title = menus.title;
    title.select(2); // DIFFICULTY
    CHECK_EQ(title.items()[2].value(), std::string{"NORMAL"});
    CHECK(title.handle(MenuInput::Right).changed);
    CHECK(settings.difficulty == game::Difficulty::Hard);
    title.select(3); // SOUND
    static_cast<void>(title.handle(MenuInput::Left));
    CHECK(settings.volume == gamelib::audio::VolumeLevel::Low);
    title.select(4); // FULLSCREEN
    static_cast<void>(title.handle(MenuInput::Confirm));
    CHECK(settings.fullscreen);
    CHECK_EQ(title.items()[4].value(), std::string{"ON"});
}

TEST_CASE("pause and game-over pages")
{
    Settings settings;
    Menus menus = buildMenus(settings);
    CHECK_EQ(menus.paused.handle(MenuInput::Back).command, static_cast<int>(Resume));
    CHECK_EQ(menus.paused.handle(MenuInput::Confirm).command, static_cast<int>(Resume));
    static_cast<void>(menus.paused.handle(MenuInput::Down));
    CHECK_EQ(menus.paused.handle(MenuInput::Confirm).command, static_cast<int>(MainMenu));
    CHECK_EQ(menus.gameOver.handle(MenuInput::Confirm).command, static_cast<int>(PlayAgain));
    CHECK_EQ(menus.gameOver.handle(MenuInput::Back).command, static_cast<int>(MainMenu));
}

TEST_CASE("settings round-trip through pong.ini and survive bad values")
{
    const TempDir dir;
    const fs::path file = dir.path() / "nested" / "pong.ini";
    SettingsFile settings = openSettings(file);
    CHECK(settings.value() == Settings{}); // missing file: defaults

    settings.value() = {.difficulty = game::Difficulty::Easy,
                        .volume = gamelib::audio::VolumeLevel::Off,
                        .fullscreen = true};
    REQUIRE(settings.save());
    CHECK(openSettings(file).value() == settings.value());
    CHECK(gamelib::storage::readFile(file).value_or("").contains("difficulty = easy"));

    REQUIRE(!gamelib::storage::writeFileAtomic(file, "[settings]\ndifficulty = impossible\nsound = HIGH\nfullscreen = maybe\n"));
    const Settings loaded = openSettings(file).value();
    CHECK(loaded.difficulty == game::Difficulty::Normal); // unknown value: default
    CHECK(loaded.volume == gamelib::audio::VolumeLevel::High); // case-insensitive
    CHECK(!loaded.fullscreen);
}

TEST_CASE("every sound effect renders, is audible and never clips")
{
    for (std::size_t i = 0; i < kSfxCount; ++i) {
        const std::vector<float> samples = renderSfx(static_cast<Sfx>(i), 48000);
        REQUIRE(!samples.empty());
        float peak = 0.0f;
        for (const float s : samples) {
            peak = std::max(peak, std::abs(s));
        }
        CHECK(peak > 0.05f);
        CHECK(peak < 0.5f); // every recipe peaks between -12 and -19 dBFS
        CHECK(samples.size() < 48000u); // all short effects, under a second
    }
}

TEST_CASE("effects: paddle flash, score pulse and shake fade on frame time")
{
    Effects effects;
    effects.onPaddleHit(game::Side::Right);
    effects.onScore(game::Side::Left);
    CHECK_NEAR(effects.paddleFlash[1].value(), 1.0f, 0.0);
    CHECK_NEAR(effects.paddleFlash[0].value(), 0.0f, 0.0);
    CHECK(gamelib::core::length(effects.shake.offset(0.37f)) > 0.0f);
    effects.update(0.1f);
    CHECK(effects.paddleFlash[1].value() < 1.0f);
    CHECK(effects.scorePulse[0].value() > 0.0f);
    effects.update(10.0f);
    CHECK(!effects.paddleFlash[1].active());
    CHECK(!effects.scorePulse[0].active());
    CHECK_EQ(effects.shake.offset(0.37f), gamelib::core::Vec2{});
    effects.trail.record({1.0f, 2.0f});
    effects.clear();
    CHECK_EQ(effects.trail.size(), std::size_t{0});
}
