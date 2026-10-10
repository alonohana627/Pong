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

To reproduce a combination: check out the Pong commit, then
`git -C ../GameLib checkout <revision>` and build. CMake prints the GameLib
revision it builds against (`Pong: using GameLib ...`).
