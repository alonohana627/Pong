#include "pong_game.hpp"

#include "court_frag.spv.hpp"
#include "digit_frag.spv.hpp"

#include <gamelib/audio/player.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

namespace pong::app {

using gamelib::platform::Key;

namespace {

// Menu keys are ignored this long after a game over, so a key meant for the
// last rally does not instantly start the next match.
constexpr float kGameOverInputDelay = 0.6f;
constexpr const char* kTitleFooter = "LEFT: W / S    RIGHT: UP / DOWN    ESC: PAUSE    F11: FULLSCREEN";

std::optional<MenuInput> _menuInput(const Key key)
{
    switch (key) {
    case Key::Up:
    case Key::W: return MenuInput::Up;
    case Key::Down:
    case Key::S: return MenuInput::Down;
    case Key::Left:
    case Key::A: return MenuInput::Left;
    case Key::Right:
    case Key::D: return MenuInput::Right;
    case Key::Enter:
    case Key::Space: return MenuInput::Confirm;
    case Key::Escape:
    case Key::Backspace: return MenuInput::Back;
    default: return std::nullopt;
    }
}

} // namespace

PongGame::PongGame(gamelib::app::App& app, const Settings& settings, std::filesystem::path settingsFile,
                   const bool demo)
    : m_settings{settings}
    , m_settingsFile{std::move(settingsFile)}
    , m_shapes{.court = app.renderer().addShape(shaders::court_frag),
               .digit = app.renderer().addShape(shaders::digit_frag)}
    , m_sounds{app.audio()}
    , m_match{game::Rules{}, m_seeds.next64()}
    , m_attract{game::Rules{}, m_seeds.next64()}
    , m_leftAi{game::Side::Left, game::Difficulty::Hard, m_seeds.next64()}
    , m_rightAi{game::Side::Right, game::Difficulty::Normal, m_seeds.next64()}
{
    if (std::error_code ignored; std::filesystem::exists(m_settingsFile, ignored)) {
        m_saved = m_settings;
    }
    save(); // a missing file is written with the defaults, so it can be found and edited
    applySettings(app);
    m_menu.open(MenuPage::Title);
    startAttract();
    if (demo) {
        startMatch(Mode::Demo);
    }
}

void PongGame::startMatch(const Mode mode)
{
    m_mode = mode;
    const game::Side firstReceiver = m_seeds.chance(0.5f) ? game::Side::Left : game::Side::Right;
    m_match = game::Match{game::Rules{}, m_seeds.next64(), firstReceiver};
    // In a one-player game the right paddle plays at the chosen difficulty.
    const game::Difficulty right = mode == Mode::OnePlayer ? m_settings.difficulty : game::Difficulty::Normal;
    m_leftAi = game::Ai{game::Side::Left, game::Difficulty::Normal, m_seeds.next64()};
    m_rightAi = game::Ai{game::Side::Right, right, m_seeds.next64()};
    m_effects.clear();
    m_screen = Screen::Playing;
}

void PongGame::startAttract()
{
    m_attract = game::Match{game::Rules{}, m_seeds.next64()};
}

game::Match& PongGame::activeMatch()
{
    return m_screen == Screen::Title ? m_attract : m_match;
}

void PongGame::onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event)
{
    if (!event.pressed) {
        return; // paddles read held keys in tick(); menus react to presses only
    }
    if (event.key == Key::F11) {
        m_settings.fullscreen = !m_settings.fullscreen;
        applySettings(app);
        save();
        return;
    }
    if (m_screen == Screen::Playing) {
        if (event.key == Key::Escape || event.key == Key::P) {
            m_screen = Screen::Paused;
            m_menu.open(MenuPage::Paused);
            app.input().releaseAll(); // releases now go to the menu
            m_sounds.queue(app.sounds(), Sfx::MenuBack);
        }
        return;
    }
    handleMenu(app, event.key);
}

void PongGame::handleMenu(gamelib::app::App& app, const Key key)
{
    const std::optional<MenuInput> input = _menuInput(key);
    if (!input || (m_screen == Screen::GameOver && m_time - m_gameOverAt < kGameOverInputDelay)) {
        return;
    }
    const MenuResult result = m_menu.handle(*input);
    switch (result.command) {
    case MenuCommand::None: break;
    case MenuCommand::StartOnePlayer: startMatch(Mode::OnePlayer); break;
    case MenuCommand::StartTwoPlayers: startMatch(Mode::TwoPlayers); break;
    case MenuCommand::PlayAgain: startMatch(m_mode); break;
    case MenuCommand::Resume: m_screen = Screen::Playing; break;
    case MenuCommand::MainMenu:
        m_screen = Screen::Title;
        m_menu.open(MenuPage::Title);
        m_effects.clear();
        break;
    case MenuCommand::Quit: app.requestQuit(); break;
    case MenuCommand::SettingsChanged:
        applySettings(app);
        save();
        break;
    }
    // Feedback after acting, so a new volume is heard right away.
    switch (result.feedback) {
    case MenuFeedback::None: break;
    case MenuFeedback::Moved: m_sounds.queue(app.sounds(), Sfx::MenuMove); break;
    case MenuFeedback::Selected: m_sounds.queue(app.sounds(), Sfx::MenuSelect); break;
    case MenuFeedback::Back: m_sounds.queue(app.sounds(), Sfx::MenuBack); break;
    }
}

void PongGame::tick(gamelib::app::App& app)
{
    const float dt = app.tickSeconds();
    if (m_screen == Screen::Title) {
        m_attract.tick({.left = m_leftAi.decide(m_attract), .right = m_rightAi.decide(m_attract)}, dt);
        return;
    }
    if (m_screen != Screen::Playing) {
        return; // paused or game over: the match is frozen
    }
    const gamelib::platform::InputState& keys = app.input();
    game::Input input;
    switch (m_mode) {
    case Mode::OnePlayer:
        input.left = static_cast<float>(keys.axis({Key::W, Key::Up}, {Key::S, Key::Down}));
        input.right = m_rightAi.decide(m_match);
        break;
    case Mode::TwoPlayers:
        input.left = static_cast<float>(keys.axis({Key::W}, {Key::S}));
        input.right = static_cast<float>(keys.axis({Key::Up}, {Key::Down}));
        break;
    case Mode::Demo:
        input.left = m_leftAi.decide(m_match);
        input.right = m_rightAi.decide(m_match);
        break;
    }
    m_match.tick(input, dt);
}

void PongGame::frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info)
{
    m_time = info.time;
    m_effects.update(info.seconds);
    handleEvents(app);
    const game::Match& match = activeMatch();
    m_effects.recordBall(match.ball().position.at(info.alpha),
                         match.phase() == game::Phase::Rally);
}

void PongGame::handleEvents(gamelib::app::App& app)
{
    if (m_screen == Screen::Title) {
        // The attract match is silent; only its effects show.
        for (const game::Event& event : m_attract.takeEvents()) {
            if (event.kind == game::EventKind::PaddleHit) {
                m_effects.onPaddleHit(event.side);
            }
        }
        if (m_attract.phase() == game::Phase::GameOver) {
            startAttract();
        }
        return;
    }
    const float speedRange = m_match.rules().maxSpeed - m_match.rules().serveSpeed;
    for (const game::Event& event : m_match.takeEvents()) {
        switch (event.kind) {
        case game::EventKind::Served: m_sounds.queue(app.sounds(), Sfx::Serve); break;
        case game::EventKind::PaddleHit: {
            m_effects.onPaddleHit(event.side);
            // Higher as the rally speeds up: up to a fifth above the start.
            const float t = std::clamp((event.speed - m_match.rules().serveSpeed) / speedRange, 0.0f, 1.0f);
            m_sounds.queue(app.sounds(), Sfx::PaddleHit, 1.0f + 0.5f * t);
            break;
        }
        case game::EventKind::WallHit: m_sounds.queue(app.sounds(), Sfx::WallHit); break;
        case game::EventKind::Scored:
            m_effects.onScore(event.side);
            m_sounds.queue(app.sounds(), Sfx::Score);
            break;
        case game::EventKind::MatchWon:
            m_sounds.queue(app.sounds(), Sfx::Win);
            if (m_mode == Mode::Demo) {
                startMatch(Mode::Demo); // endless
            } else {
                m_screen = Screen::GameOver;
                m_menu.open(MenuPage::GameOver);
                m_gameOverAt = m_time;
                app.input().releaseAll();
            }
            break;
        }
    }
}

void PongGame::draw(gamelib::app::App&, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    drawCourt(list, m_shapes, info.time);

    std::string_view left = "CPU";
    std::string_view right = "CPU";
    if (m_screen != Screen::Title) {
        if (m_mode == Mode::OnePlayer) {
            left = "YOU";
        } else if (m_mode == Mode::TwoPlayers) {
            left = "PLAYER 1";
            right = "PLAYER 2";
        }
    }
    const game::Match& match = activeMatch();
    drawMatch(list, m_shapes,
              {.match = match,
               .effects = m_effects,
               .alpha = m_screen == Screen::Playing || m_screen == Screen::Title ? info.alpha : 1.0f,
               .time = info.time,
               .leftName = left,
               .rightName = right,
               .dim = m_screen != Screen::Playing});

    switch (m_screen) {
    case Screen::Playing: drawServeHint(list, match, info.time); break;
    case Screen::Title: drawMenu(list, {.menu = m_menu, .title = "PONG", .subtitle = {}, .footer = kTitleFooter}); break;
    case Screen::Paused: drawMenu(list, {.menu = m_menu, .title = "PAUSED", .subtitle = {}, .footer = {}}); break;
    case Screen::GameOver: {
        const game::Side winner = m_match.winner().value_or(game::Side::Left);
        std::string_view title = winner == game::Side::Left ? "LEFT WINS" : "RIGHT WINS";
        if (m_mode == Mode::OnePlayer) {
            title = winner == game::Side::Left ? "YOU WIN!" : "CPU WINS";
        } else if (m_mode == Mode::TwoPlayers) {
            title = winner == game::Side::Left ? "PLAYER 1 WINS" : "PLAYER 2 WINS";
        }
        const std::string score =
            std::to_string(m_match.score(game::Side::Left)) + " - " + std::to_string(m_match.score(game::Side::Right));
        drawMenu(list, {.menu = m_menu, .title = title, .subtitle = score, .footer = {}});
        break;
    }
    }
}

void PongGame::applySettings(gamelib::app::App& app)
{
    app.setFullscreen(m_settings.fullscreen);
    app.audio().setMasterVolume(gamelib::audio::gain(m_settings.volume));
}

void PongGame::save()
{
    // Only when something changed since the last successful write.
    if (m_saved == m_settings) {
        return;
    }
    if (saveSettings(m_settingsFile, m_settings)) {
        m_saved = m_settings;
    }
}

} // namespace pong::app
