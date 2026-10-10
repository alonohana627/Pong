#include "pong_game.hpp"

#include "court_frag.spv.hpp"
#include "palette.hpp"

#include <gamelib/audio/player.hpp>
#include <gamelib/ui/menu.hpp>

#include <algorithm>
#include <string>
#include <utility>

namespace pong::app {

using gamelib::platform::Key;

namespace {

// Menu keys are ignored this long after a game over, so a key meant for the
// last rally does not instantly start the next match.
constexpr float kGameOverInputDelay = 0.6f;

Demo _makeDemo(const std::uint64_t seed)
{
    return Demo{
        .match = game::Match{game::Rules{}, seed},
        .left = game::Ai{game::Side::Left, game::Difficulty::Hard, seed * 31 + 1},
        .right = game::Ai{game::Side::Right, game::Difficulty::Normal, seed * 57 + 3},
    };
}

} // namespace

PongGame::PongGame(gamelib::app::App& app, SettingsFile settings, const bool demo)
    : m_settings{std::move(settings)}
    , m_menus{buildMenus(m_settings.value())}
    , m_shapes{.court = app.renderer().addShape(shaders::court_frag)}
    , m_sounds{app.audio(), renderSfx}
    , m_attract{_makeDemo,
                [](Demo& d, const float dt) {
                    d.match.tick({.left = d.left.decide(d.match), .right = d.right.decide(d.match)}, dt);
                },
                [](const Demo& d) { return d.match.phase() == game::Phase::GameOver; }, m_seeds.next64()}
    , m_match{game::Rules{}, m_seeds.next64()}
    , m_leftAi{game::Side::Left, game::Difficulty::Normal, m_seeds.next64()}
    , m_rightAi{game::Side::Right, game::Difficulty::Normal, m_seeds.next64()}
{
    m_settings.save(); // a missing file is written with the defaults, so it can be found and edited
    applySettings(app);
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
    const game::Difficulty right = mode == Mode::OnePlayer ? m_settings.value().difficulty : game::Difficulty::Normal;
    m_leftAi = game::Ai{game::Side::Left, game::Difficulty::Normal, m_seeds.next64()};
    m_rightAi = game::Ai{game::Side::Right, right, m_seeds.next64()};
    m_effects.clear();
    showScreen(Screen::Playing);
}

void PongGame::showScreen(const Screen screen)
{
    m_screens.go(screen, m_time);
    activeMenu().select(0);
}

gamelib::ui::Menu& PongGame::activeMenu()
{
    switch (m_screens.current()) {
    case Screen::Paused: return m_menus.paused;
    case Screen::GameOver: return m_menus.gameOver;
    case Screen::Title:
    case Screen::Playing: break;
    }
    return m_menus.title;
}

const game::Match& PongGame::shownMatch() const
{
    return m_screens.is(Screen::Title) ? m_attract.sim().match : m_match;
}

void PongGame::onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event)
{
    if (!event.pressed) {
        return; // paddles read held keys in tick(); menus react to presses only
    }
    if (event.key == Key::F11) {
        m_settings.value().fullscreen = !m_settings.value().fullscreen;
        applySettings(app);
        m_settings.save();
        return;
    }
    if (m_screens.is(Screen::Playing)) {
        if (event.key == Key::Escape || event.key == Key::P) {
            showScreen(Screen::Paused);
            app.input().releaseAll(); // releases now go to the menu
            m_sounds.queue(app.sounds(), Sfx::MenuBack);
        }
        return;
    }
    handleMenu(app, event.key);
}

void PongGame::handleMenu(gamelib::app::App& app, const Key key)
{
    const std::optional<gamelib::ui::MenuInput> input = gamelib::ui::menuInput(key);
    if (!input || (m_screens.is(Screen::GameOver) && !m_screens.inputReady(m_time, kGameOverInputDelay))) {
        return;
    }
    const gamelib::ui::MenuResult result = activeMenu().handle(*input);
    if (result.changed) {
        applySettings(app);
        m_settings.save();
    }
    switch (result.command) {
    case StartOnePlayer: startMatch(Mode::OnePlayer); break;
    case StartTwoPlayers: startMatch(Mode::TwoPlayers); break;
    case PlayAgain: startMatch(m_mode); break;
    case Resume: m_screens.go(Screen::Playing, m_time); break;
    case MainMenu:
        showScreen(Screen::Title);
        m_effects.clear();
        break;
    case Quit: app.requestQuit(); break;
    default: break;
    }
    // Feedback after acting, so a new volume is heard right away.
    switch (result.feedback) {
    case gamelib::ui::MenuFeedback::None: break;
    case gamelib::ui::MenuFeedback::Moved:
    case gamelib::ui::MenuFeedback::Changed: m_sounds.queue(app.sounds(), Sfx::MenuMove); break;
    case gamelib::ui::MenuFeedback::Selected: m_sounds.queue(app.sounds(), Sfx::MenuSelect); break;
    case gamelib::ui::MenuFeedback::Back: m_sounds.queue(app.sounds(), Sfx::MenuBack); break;
    }
}

void PongGame::tick(gamelib::app::App& app)
{
    const float dt = app.tickSeconds();
    if (m_screens.is(Screen::Title)) {
        m_attract.tick(dt);
        return;
    }
    if (!m_screens.is(Screen::Playing)) {
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
    const game::Match& match = shownMatch();
    if (match.phase() == game::Phase::Rally) {
        m_effects.trail.record(match.ball().position.at(info.alpha));
    } else {
        m_effects.trail.clear(); // a serve breaks the trail
    }
}

void PongGame::handleEvents(gamelib::app::App& app)
{
    if (m_screens.is(Screen::Title)) {
        // The attract match is silent; only its paddle flashes show.
        for (const game::Event& event : m_attract.sim().match.takeEvents()) {
            if (event.kind == game::EventKind::PaddleHit) {
                m_effects.onPaddleHit(event.side);
            }
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
            m_sounds.queue(app.sounds(), Sfx::PaddleHit, 1.0f, 1.0f + 0.5f * t);
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
                showScreen(Screen::GameOver);
                app.input().releaseAll();
            }
            break;
        }
    }
}

void PongGame::draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    drawCourt(list, m_shapes);

    std::string_view left = "CPU";
    std::string_view right = "CPU";
    if (!m_screens.is(Screen::Title)) {
        if (m_mode == Mode::OnePlayer) {
            left = "YOU";
        } else if (m_mode == Mode::TwoPlayers) {
            left = "PLAYER 1";
            right = "PLAYER 2";
        }
    }
    const bool moving = m_screens.is(Screen::Playing) || m_screens.is(Screen::Title);
    const game::Match& match = shownMatch();
    drawMatch(list, {.match = match,
                     .effects = m_effects,
                     .alpha = moving ? info.alpha : 1.0f,
                     .time = info.time,
                     .leftName = left,
                     .rightName = right,
                     .dim = !m_screens.is(Screen::Playing)});

    const gamelib::core::Size canvas = app.canvas();
    switch (m_screens.current()) {
    case Screen::Playing: drawServeHint(list, match, info.time); break;
    case Screen::Title: gamelib::ui::drawMenu(list, m_menus.title, canvas, {}, {}, kTitleFooter); break;
    case Screen::Paused: gamelib::ui::drawMenu(list, m_menus.paused, canvas); break;
    case Screen::GameOver: {
        const game::Side winner = m_match.winner().value_or(game::Side::Left);
        std::string title = winner == game::Side::Left ? "LEFT WINS" : "RIGHT WINS";
        if (m_mode == Mode::OnePlayer) {
            title = winner == game::Side::Left ? "YOU WIN!" : "CPU WINS";
        } else if (m_mode == Mode::TwoPlayers) {
            title = winner == game::Side::Left ? "PLAYER 1 WINS" : "PLAYER 2 WINS";
        }
        m_menus.gameOver.setTitle(title);
        const std::string score =
            std::to_string(m_match.score(game::Side::Left)) + " - " + std::to_string(m_match.score(game::Side::Right));
        gamelib::ui::drawMenu(list, m_menus.gameOver, canvas, {}, score);
        break;
    }
    }
}

void PongGame::applySettings(gamelib::app::App& app)
{
    app.setFullscreen(m_settings.value().fullscreen);
    app.audio().setMasterVolume(gamelib::audio::gain(m_settings.value().volume));
}

} // namespace pong::app
