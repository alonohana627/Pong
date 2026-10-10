# Skill suggestions

Skills that would have made work on this project go better. See `AGENTS.md` for
how this file is maintained.

## Already installed

In `~/.claude/skills/` (checked 2026-10-10):

- chiptune-sfx-synthesis
- ci-cross-platform
- cmake-build-developer
- cmake-library-consumption (suggested in GameLib)
- computer-graphics-developer
- cpp-debugging-profiling
- cpp-testing
- cpp26-developer
- game-audio-developer
- game-engine-developer
- game-feel-designer
- game-library-extraction (suggested in GameLib)
- game-persistence
- game-visual-design
- glfw-platform-developer
- glsl-shader-build-embedding
- learning-mentor
- sdf-2d-shape-shaders
- shader-developer
- software-architecture-developer
- technical-docs-writer
- vulkan-app-smoke-testing (suggested in GameLib)
- vulkan-developer
- vulkan-text-and-ui
- win32-game-window-automation
- windows-release-packaging

## Suggested

Ordered by value for the next steps.

### pong-arcade-rules

Classic Pong rules and feel as concrete numbers and algorithms: court
proportions, paddle and ball sizes relative to the court, serve rules (who
serves, delay, launch angle range), the bounce angle as a function of the
contact point on the paddle, speed-up per rally hit and its cap, avoiding
tunneling with swept collisions, scoring to 11 with a two-point lead option,
and beatable AI opponents (reaction delay, prediction of the bounce off walls,
capped paddle speed, deliberate aim error by difficulty).

Why: Pong's rules and AI tuning (bounce angle, speed-up, serve, AI
reaction and aim error) had to be worked out from scratch; `game-feel-designer` covers Snake,
Tetris, Breakout and Pac-Man style rules but has no Pong numbers or AI recipe.
