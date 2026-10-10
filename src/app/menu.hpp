#pragma once

#include "settings.hpp"

#include <span>
#include <string>
#include <string_view>

namespace pong::app {

// Menu logic: pages, items and the selection. It knows nothing about drawing
// (view.cpp does that) or keys (PongGame maps keys to MenuInput); it reports
// what the player chose as a MenuCommand. Settings items edit the Settings it
// was given directly and report SettingsChanged.

enum class MenuPage { Title, Paused, GameOver };

enum class MenuItem { OnePlayer, TwoPlayers, Difficulty, Sound, Fullscreen, Quit, Resume, MainMenu, PlayAgain };

enum class MenuInput { Up, Down, Left, Right, Confirm, Back };

enum class MenuCommand { None, StartOnePlayer, StartTwoPlayers, Resume, PlayAgain, MainMenu, Quit, SettingsChanged };

// What the menu did with an input, so the game can give feedback (sounds).
enum class MenuFeedback { None, Moved, Selected, Back };

struct MenuResult {
    MenuCommand command = MenuCommand::None;
    MenuFeedback feedback = MenuFeedback::None;
};

class Menu {
public:
    explicit Menu(Settings& settings);

    // Shows a page with its first item selected.
    void open(MenuPage page);
    MenuResult handle(MenuInput input);

    [[nodiscard]] MenuPage page() const { return m_page; }
    [[nodiscard]] std::span<const MenuItem> items() const;
    [[nodiscard]] std::size_t selected() const { return m_selected; }
    [[nodiscard]] const Settings& settings() const { return m_settings; }

private:
    MenuResult activate(MenuItem item);
    // Steps a setting forwards (+1) or backwards (-1), wrapping around.
    MenuResult change(MenuItem item, int step);

    Settings& m_settings;
    MenuPage m_page = MenuPage::Title;
    std::size_t m_selected = 0;
};

[[nodiscard]] std::string_view label(MenuItem item);
// The current value of a settings item ("NORMAL", "ON"...), "" for others.
[[nodiscard]] std::string value(MenuItem item, const Settings& settings);

} // namespace pong::app
