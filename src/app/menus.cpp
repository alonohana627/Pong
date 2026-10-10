#include "menus.hpp"

namespace pong::app {

Menus buildMenus(Settings& settings)
{
    Menus menus{
        .title = gamelib::ui::Menu{"PONG"},
        .paused = gamelib::ui::Menu{"PAUSED", Resume},      // Escape resumes
        .gameOver = gamelib::ui::Menu{"GAME OVER", MainMenu}, // Escape leaves
    };
    menus.title.action("1 PLAYER", StartOnePlayer)
        .action("2 PLAYERS", StartTwoPlayers)
        .option("DIFFICULTY", settings.difficulty, game::kDifficulties)
        .option("SOUND", settings.volume, kVolumes)
        .toggle("FULLSCREEN", settings.fullscreen)
        .action("QUIT", Quit);
    menus.paused.action("RESUME", Resume).action("MAIN MENU", MainMenu).action("QUIT", Quit);
    menus.gameOver.action("PLAY AGAIN", PlayAgain).action("MAIN MENU", MainMenu);
    return menus;
}

} // namespace pong::app
