# pong_game: the rules and the computer player. Depends only on GameLib::core
# (math, collision, RNG) - no window, GPU or audio - so it is tested headless.
add_library(pong_game STATIC
    ai.cpp
    match.cpp)
target_include_directories(pong_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pong_game PUBLIC GameLib::core)
gamelib_target_warnings(pong_game)
