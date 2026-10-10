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
