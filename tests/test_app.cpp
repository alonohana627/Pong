// Headless tests of Pong's presentation logic: menu, settings file, sound
// recipes, score digits and effects. No window, GPU or audio device.
#include "effects.hpp"
#include "menu.hpp"
#include "settings.hpp"
#include "sounds.hpp"
#include "view.hpp"

#include <gamelib/core/rng.hpp>
#include <gamelib/storage/files.hpp>
#include <gamelib/testing/test.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <filesystem>
#include <format>

using namespace pong;
using namespace pong::app;
namespace fs = std::filesystem;

namespace {

class TempDir {
public:
    TempDir()
        : m_path{fs::temp_directory_path() / std::format("pong-test-{:016x}", gamelib::core::randomSeed())}
    {
        fs::create_directories(m_path);
    }
    ~TempDir()
    {
        std::error_code ignored;
        fs::remove_all(m_path, ignored);
    }
    TempDir(const TempDir&) = delete("owns a directory");
    TempDir& operator=(const TempDir&) = delete("owns a directory");
    [[nodiscard]] const fs::path& path() const { return m_path; }

private:
    fs::path m_path;
};

} // namespace

TEST_CASE("title menu starts games and wraps the selection")
{
    Settings settings;
    Menu menu{settings};
    menu.open(MenuPage::Title);
    CHECK(menu.items()[menu.selected()] == MenuItem::OnePlayer);
    CHECK(menu.handle(MenuInput::Confirm).command == MenuCommand::StartOnePlayer);

    const MenuResult up = menu.handle(MenuInput::Up); // wraps to the last item
    CHECK(up.feedback == MenuFeedback::Moved);
    CHECK(menu.items()[menu.selected()] == MenuItem::Quit);
    CHECK(menu.handle(MenuInput::Confirm).command == MenuCommand::Quit);
    static_cast<void>(menu.handle(MenuInput::Down)); // wraps back to the first
    static_cast<void>(menu.handle(MenuInput::Down));
    CHECK(menu.handle(MenuInput::Confirm).command == MenuCommand::StartTwoPlayers);
    CHECK(menu.handle(MenuInput::Back).command == MenuCommand::None); // nothing to go back to
}

TEST_CASE("menu settings items change the settings in both directions")
{
    Settings settings;
    Menu menu{settings};
    menu.open(MenuPage::Title);
    static_cast<void>(menu.handle(MenuInput::Down));
    static_cast<void>(menu.handle(MenuInput::Down)); // DIFFICULTY
    REQUIRE(menu.items()[menu.selected()] == MenuItem::Difficulty);
    CHECK(menu.handle(MenuInput::Right).command == MenuCommand::SettingsChanged);
    CHECK(settings.difficulty == game::Difficulty::Hard);
    static_cast<void>(menu.handle(MenuInput::Right)); // wraps around
    CHECK(settings.difficulty == game::Difficulty::Easy);
    static_cast<void>(menu.handle(MenuInput::Left));
    CHECK(settings.difficulty == game::Difficulty::Hard);
    CHECK_EQ(value(MenuItem::Difficulty, settings), std::string{"HARD"});

    static_cast<void>(menu.handle(MenuInput::Down)); // SOUND
    static_cast<void>(menu.handle(MenuInput::Confirm)); // Enter cycles too
    CHECK(settings.volume == gamelib::audio::VolumeLevel::High);
    static_cast<void>(menu.handle(MenuInput::Down)); // FULLSCREEN
    static_cast<void>(menu.handle(MenuInput::Left));
    CHECK(settings.fullscreen);
    CHECK_EQ(value(MenuItem::Fullscreen, settings), std::string{"ON"});
    CHECK(menu.handle(MenuInput::Down).command == MenuCommand::None);
    CHECK(menu.handle(MenuInput::Left).command == MenuCommand::None); // QUIT is not a setting
}

TEST_CASE("pause and game-over pages")
{
    Settings settings;
    Menu menu{settings};
    menu.open(MenuPage::Paused);
    CHECK(menu.handle(MenuInput::Back).command == MenuCommand::Resume);
    CHECK(menu.handle(MenuInput::Confirm).command == MenuCommand::Resume);
    static_cast<void>(menu.handle(MenuInput::Down));
    CHECK(menu.handle(MenuInput::Confirm).command == MenuCommand::MainMenu);

    menu.open(MenuPage::GameOver);
    CHECK(menu.selected() == 0);
    CHECK(menu.handle(MenuInput::Confirm).command == MenuCommand::PlayAgain);
    CHECK(menu.handle(MenuInput::Back).command == MenuCommand::MainMenu);
    CHECK_EQ(label(MenuItem::PlayAgain), std::string_view{"PLAY AGAIN"});
}

TEST_CASE("settings round-trip through pong.ini and survive bad values")
{
    const TempDir dir;
    const fs::path file = dir.path() / "nested" / "pong.ini";
    CHECK(loadSettings(file) == Settings{}); // missing file: defaults

    const Settings custom{.difficulty = game::Difficulty::Easy,
                          .volume = gamelib::audio::VolumeLevel::Off,
                          .fullscreen = true};
    REQUIRE(saveSettings(file, custom));
    CHECK(loadSettings(file) == custom);
    CHECK(gamelib::storage::readFile(file).value_or("").contains("difficulty = easy"));

    REQUIRE(!gamelib::storage::writeFileAtomic(file, "[settings]\ndifficulty = impossible\nsound = HIGH\nfullscreen = maybe\n"));
    const Settings loaded = loadSettings(file);
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

TEST_CASE("seven-segment masks")
{
    CHECK_EQ(segmentMask(8), 0x7Fu); // all seven segments
    CHECK_EQ(segmentMask(1), 0x06u); // the two right-hand segments
    CHECK_EQ(std::popcount(segmentMask(0)), 6); // all but the middle
    CHECK_EQ(segmentMask(0) & 0x40u, 0u);
    CHECK_EQ(segmentMask(-1), 0x40u); // a dash
    CHECK_EQ(segmentMask(10), 0x40u);
    for (int d = 0; d < 10; ++d) {
        for (int e = d + 1; e < 10; ++e) {
            CHECK(segmentMask(d) != segmentMask(e));
        }
    }
}

TEST_CASE("effects fade, shake and keep a bounded trail")
{
    Effects effects;
    CHECK_EQ(effects.shakeOffset(1.0f), gamelib::core::Vec2{});
    effects.onPaddleHit(game::Side::Right);
    effects.onScore(game::Side::Left);
    CHECK_NEAR(effects.paddleFlash(game::Side::Right), 1.0f, 0.0);
    CHECK_NEAR(effects.paddleFlash(game::Side::Left), 0.0f, 0.0);
    CHECK(gamelib::core::length(effects.shakeOffset(0.37f)) > 0.0f);
    effects.update(0.1f);
    CHECK(effects.paddleFlash(game::Side::Right) < 1.0f);
    effects.update(10.0f);
    CHECK_NEAR(effects.paddleFlash(game::Side::Right), 0.0f, 0.0);
    CHECK_NEAR(effects.scorePulse(game::Side::Left), 0.0f, 0.0);
    CHECK_EQ(effects.shakeOffset(0.37f), gamelib::core::Vec2{});

    for (int i = 0; i < 25; ++i) {
        effects.recordBall({static_cast<float>(i), 0.0f}, true);
    }
    CHECK_EQ(effects.trailCount(), Effects::kTrailLength);
    CHECK_NEAR(effects.trail()[0].x, 24.0f, 0.0); // newest first
    CHECK_NEAR(effects.trail()[1].x, 23.0f, 0.0);
    effects.recordBall({}, false); // a serve breaks the trail
    CHECK_EQ(effects.trailCount(), std::size_t{0});
}
