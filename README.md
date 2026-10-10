# Pong

The classic, rebuilt as the first game on [GameLib](https://github.com/alonohana627/GameLib): two paddles,
one ball, first to 11. Everything you see is drawn by Vulkan shaders - glowing
SDF paddles and ball, a pulsing net, seven-segment score digits - and every
sound is synthesized. There are no image or audio files.

## Play

| | |
|---|---|
| 1 player | you play the left paddle (W/S or Up/Down) against the computer |
| 2 players | left W/S, right Up/Down |
| Difficulty | easy / normal / hard computer opponent (title menu) |
| Pause | Esc or P |
| Fullscreen | F11, or the title menu |

The title screen plays a computer-vs-computer match behind the menu;
`pong.exe --demo` plays one endlessly. Settings are saved to
`%APPDATA%\Pong\pong.ini` (`PONG_DATA_DIR` overrides the folder).

## Build

Pong downloads GameLib from GitHub at a pinned release tag (CMake
FetchContent, into the build folder) and builds it as part of its own build;
no install step. Requirements are GameLib's: MSYS2
CLANG64 with Clang, CMake 3.28+, Ninja, glslc, a Vulkan 1.3 driver - see
GameLib's `docs/GETTING_STARTED.md`.

```powershell
cmake --preset debug
cmake --build --preset debug
build/debug/bin/pong.exe
ctest --preset debug            # gameplay tests and three smoke runs
ctest --preset debug-headless   # gameplay tests only
```

A local GameLib checkout instead (for developing both together):
`cmake --preset debug -DFETCHCONTENT_SOURCE_DIR_GAMELIB=C:/Users/user/GameLib`.
The GameLib revision each Pong change was validated with is in
[docs/GAMELIB_REVISION.md](docs/GAMELIB_REVISION.md).

## Layout

| Path | What |
|---|---|
| `src/game/` | rules (`match.*`) and the computer player (`ai.*`); depends only on `GameLib::core`, fully unit-tested |
| `src/app/` | `PongGame` (GameLib main-loop glue), menu, settings, sounds, effects, views |
| `shaders/` | Pong's own shapes: `court.frag`, `digit.frag` |
| `tests/` | gameplay and app tests (headless) and smoke runs of the real game |

What Pong can do is listed in [FEATURES.md](FEATURES.md); working rules for
agents are in [AGENTS.md](AGENTS.md).
