# Features

What Pong does today. Only implemented, tested features are listed; updated
in the same commit as each feature (see `AGENTS.md`).

## Rules (`src/game`)

- Two paddles, one ball, walls at the top and bottom, a point when the ball
  leaves the court; first to 11 wins (configurable).
- Serve after a one-second pause, towards the player who lost the point, at
  a random angle within +-28 degrees.
- Paddle returns angled by where the ball hits (up to 55 degrees), 6% faster
  per return up to a speed cap, with a little spin from a moving paddle.
- Swept collisions: a fast ball cannot pass through a paddle between ticks.
- Deterministic fixed-tick simulation (same seed and inputs, same game) with
  previous positions for smooth interpolated drawing.
- Computer opponent with three difficulties (easy, normal, hard): predicts the
  ball's path off the walls, reacts with a delay, aims with an error that grows
  with ball speed, and moves at a capped speed - beatable on every level.

## Game (`src/app`)

- Modes: 1 player against the computer (left paddle, W/S or Up/Down),
  2 players on one keyboard (W/S and Up/Down), and an endless
  computer-vs-computer demo (`--demo`).
- Screens: title menu over a live computer-vs-computer match, playing,
  pause menu (Esc/P), game-over screen with the final score; a short input
  grace period after a match ends.
- Menu settings: difficulty, sound volume (off/low/medium/high), fullscreen;
  F11 toggles fullscreen anywhere. Saved to `pong.ini` in the user data folder
  (`PONG_DATA_DIR` overrides it).
- Visuals, all procedural shaders: a court with glowing pulsing net and faint
  grid (custom `court.frag`), seven-segment score digits with glow (custom
  `digit.frag`), glowing rounded paddles that flash on hits, a glowing ball
  with a trail, a closing ring and arrow before each serve, screen shake and a
  digit pulse on points, smooth interpolated motion at any frame rate.
- Synthesized sound effects: paddle hit (higher as the rally speeds up), wall,
  serve, point, match win, menu move/select/back.

## Tests

- Headless: rules, AI, menu, settings file, sound recipes, score digits,
  effects.
- Smoke runs of the real game (hidden window, validation layer required):
  title screen, a demo match, resize plus minimize/restore.
