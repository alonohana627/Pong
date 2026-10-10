#include "menu.hpp"

namespace pong::app {

namespace {

constexpr MenuItem kTitleItems[] = {MenuItem::OnePlayer, MenuItem::TwoPlayers, MenuItem::Difficulty,
                                    MenuItem::Sound,     MenuItem::Fullscreen, MenuItem::Quit};
constexpr MenuItem kPausedItems[] = {MenuItem::Resume, MenuItem::MainMenu, MenuItem::Quit};
constexpr MenuItem kGameOverItems[] = {MenuItem::PlayAgain, MenuItem::MainMenu};

// The next or previous value of an enum with `count` values, wrapping around.
template <typename Enum>
Enum _step(const Enum value, const int step, const int count)
{
    return static_cast<Enum>((static_cast<int>(value) + step + count) % count);
}

} // namespace

Menu::Menu(Settings& settings)
    : m_settings{settings}
{
}

void Menu::open(const MenuPage page)
{
    m_page = page;
    m_selected = 0;
}

std::span<const MenuItem> Menu::items() const
{
    switch (m_page) {
    case MenuPage::Title: return kTitleItems;
    case MenuPage::Paused: return kPausedItems;
    case MenuPage::GameOver: return kGameOverItems;
    }
    return {};
}

MenuResult Menu::handle(const MenuInput input)
{
    const std::size_t count = items().size();
    const MenuItem item = items()[m_selected];
    switch (input) {
    case MenuInput::Up:
        m_selected = (m_selected + count - 1) % count; // up from the first wraps to the last
        return {.feedback = MenuFeedback::Moved};
    case MenuInput::Down:
        m_selected = (m_selected + 1) % count;
        return {.feedback = MenuFeedback::Moved};
    case MenuInput::Left: return change(item, -1);
    case MenuInput::Right: return change(item, +1);
    case MenuInput::Confirm: return activate(item);
    case MenuInput::Back:
        // Escape resumes from the pause menu and leaves the game-over screen.
        if (m_page == MenuPage::Paused) {
            return {.command = MenuCommand::Resume, .feedback = MenuFeedback::Back};
        }
        if (m_page == MenuPage::GameOver) {
            return {.command = MenuCommand::MainMenu, .feedback = MenuFeedback::Back};
        }
        return {};
    }
    return {};
}

MenuResult Menu::activate(const MenuItem item)
{
    const auto selected = [](const MenuCommand command) {
        return MenuResult{.command = command, .feedback = MenuFeedback::Selected};
    };
    switch (item) {
    case MenuItem::OnePlayer: return selected(MenuCommand::StartOnePlayer);
    case MenuItem::TwoPlayers: return selected(MenuCommand::StartTwoPlayers);
    case MenuItem::Resume: return selected(MenuCommand::Resume);
    case MenuItem::PlayAgain: return selected(MenuCommand::PlayAgain);
    case MenuItem::MainMenu: return selected(MenuCommand::MainMenu);
    case MenuItem::Quit: return selected(MenuCommand::Quit);
    case MenuItem::Difficulty:
    case MenuItem::Sound:
    case MenuItem::Fullscreen: return change(item, +1); // Enter cycles through the values
    }
    return {};
}

MenuResult Menu::change(const MenuItem item, const int step)
{
    switch (item) {
    case MenuItem::Difficulty:
        m_settings.difficulty = _step(m_settings.difficulty, step, game::kDifficultyCount);
        break;
    case MenuItem::Sound: m_settings.volume = _step(m_settings.volume, step, gamelib::audio::kVolumeLevelCount); break;
    case MenuItem::Fullscreen: m_settings.fullscreen = !m_settings.fullscreen; break;
    default: return {}; // not a setting: left/right do nothing
    }
    return {.command = MenuCommand::SettingsChanged, .feedback = MenuFeedback::Moved};
}

std::string_view label(const MenuItem item)
{
    switch (item) {
    case MenuItem::OnePlayer: return "1 PLAYER";
    case MenuItem::TwoPlayers: return "2 PLAYERS";
    case MenuItem::Difficulty: return "DIFFICULTY";
    case MenuItem::Sound: return "SOUND";
    case MenuItem::Fullscreen: return "FULLSCREEN";
    case MenuItem::Quit: return "QUIT";
    case MenuItem::Resume: return "RESUME";
    case MenuItem::MainMenu: return "MAIN MENU";
    case MenuItem::PlayAgain: return "PLAY AGAIN";
    }
    return "";
}

std::string value(const MenuItem item, const Settings& settings)
{
    switch (item) {
    case MenuItem::Difficulty: return std::string{game::name(settings.difficulty)};
    case MenuItem::Sound: return std::string{gamelib::audio::name(settings.volume)};
    case MenuItem::Fullscreen: return settings.fullscreen ? "ON" : "OFF";
    default: return {};
    }
}

} // namespace pong::app
