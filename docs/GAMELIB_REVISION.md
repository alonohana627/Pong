# GameLib revision

Pong and GameLib are separate repositories, and commits across repositories
are not atomic. This file records which GameLib revision each Pong change was
validated against (build, `ctest --preset debug`, smoke tests). When a change
spans both, GameLib is committed first, then Pong with the new revision here.

| Pong change | GameLib revision | Validated |
|---|---|---|
| Rules and AI (`feat(game)`) | `5cf3638` | build, 14 gameplay tests |
| Playable game (`feat(app)`) | `c893b22` | build, 2 test suites, 3 smoke runs with validation |
| README (`docs`) | `fd33b9b` | fresh clones of both repositories: GameLib 14/14 CTest, Pong 5/5 CTest in debug and release |
| GameLib from GitHub (`build`) | `0.0.1` (`fd33b9b`) | clean build fetching the tag, 5/5 CTest |
| Pin GameLib main before 0.0.2 (`build`) | `9ac8627` | clean build fetching the commit, 5/5 CTest |

Since the GitHub switch, `CMakeLists.txt` pins GameLib by tag (`GIT_TAG`), so
checking out a Pong commit reproduces its combination by itself. For older
rows, configure with `-DFETCHCONTENT_SOURCE_DIR_GAMELIB=<GameLib checkout at
that revision>`. CMake prints the GameLib revision it builds against
(`Pong: using GameLib ...`).
