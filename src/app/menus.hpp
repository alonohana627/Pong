#pragma once

#include "settings.hpp"

#include <gamelib/ui/menu.hpp>

namespace pong::app {

// What Pong's menu items do (GameLib ui::Menu commands).
enum Command : int { StartOnePlayer, StartTwoPlayers, Quit, Resume, MainMenu, PlayAgain };

// Pong's three menu pages, built on GameLib's ui::Menu. The settings items
// edit `settings` in place, so it must outlive the menus.
struct Menus {
    gamelib::ui::Menu title;
    gamelib::ui::Menu paused;
    gamelib::ui::Menu gameOver;
};

[[nodiscard]] Menus buildMenus(Settings& settings);

inline constexpr const char* kTitleFooter = "LEFT: W / S    RIGHT: UP / DOWN    ESC: PAUSE    F11: FULLSCREEN";

} // namespace pong::app
