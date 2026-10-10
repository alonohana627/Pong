# pong_game: the rules and the computer player. Depends only on GameLib::core
# and GameLib::ai (math, collision, RNG, timers, computer-player helpers) - no
# window, GPU or audio - so it is tested headless.
add_library(pong_game STATIC
    ai.cpp
    match.cpp)
target_include_directories(pong_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pong_game PUBLIC GameLib::core GameLib::ai)
gamelib_target_warnings(pong_game)
