# Pong's tests. Gameplay tests are headless (label "cpu"); smoke tests launch
# the real game in GameLib's self-test mode (label "gpu").

add_executable(pong_game_tests test_match.cpp test_ai.cpp)
target_link_libraries(pong_game_tests PRIVATE pong_game GameLib::testing)
gamelib_target_warnings(pong_game_tests)
gamelib_copy_runtime(pong_game_tests)
add_test(NAME pong_game_tests COMMAND pong_game_tests)
set_tests_properties(pong_game_tests PROPERTIES LABELS cpu TIMEOUT 120)
