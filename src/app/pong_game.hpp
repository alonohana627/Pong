#pragma once

#include "ai.hpp"
#include "effects.hpp"
#include "match.hpp"
#include "menu.hpp"
#include "settings.hpp"
#include "sounds.hpp"
#include "view.hpp"

#include <gamelib/app/app.hpp>
#include <gamelib/core/rng.hpp>

#include <filesystem>
#include <optional>

namespace pong::app {

// Who controls the paddles.
enum class Mode {
    OnePlayer,  // the player on the left, the computer on the right
    TwoPlayers, // W/S on the left, Up/Down on the right
    Demo,       // computer against computer, endless (--demo, and behind the title menu)
};

// Which screen is showing. Only Playing advances the player's match; the
// title screen runs a computer-vs-computer match behind the menu.
enum class Screen { Title, Playing, Paused, GameOver };

// Pong on GameLib's main loop: maps keys to paddles and menus, ticks the
// match, turns its events into sounds and effects, and draws everything.
class PongGame final : public gamelib::app::Game {
public:
    // demo: start straight into an endless computer-vs-computer match.
    PongGame(gamelib::app::App& app, const Settings& settings, std::filesystem::path settingsFile, bool demo);

    void onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event) override;
    void tick(gamelib::app::App& app) override;
    void frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info) override;
    void draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info) override;

    [[nodiscard]] Screen screen() const { return m_screen; }

private:
    void startMatch(Mode mode);
    void startAttract();
    void handleMenu(gamelib::app::App& app, gamelib::platform::Key key);
    void handleEvents(gamelib::app::App& app);
    void applySettings(gamelib::app::App& app);
    void save();
    [[nodiscard]] game::Match& activeMatch();

    Settings m_settings;
    std::filesystem::path m_settingsFile;
    std::optional<Settings> m_saved; // what the file contains, if known
    Menu m_menu{m_settings};
    Shapes m_shapes;
    Sounds m_sounds;
    Effects m_effects;
    gamelib::core::Rng m_seeds{gamelib::core::randomSeed()};

    Screen m_screen = Screen::Title;
    Mode m_mode = Mode::OnePlayer;
    game::Match m_match;   // the player's match (or the --demo match)
    game::Match m_attract; // behind the title menu
    game::Ai m_leftAi;
    game::Ai m_rightAi;
    float m_time = 0.0f;
    float m_gameOverAt = 0.0f;
};

} // namespace pong::app
