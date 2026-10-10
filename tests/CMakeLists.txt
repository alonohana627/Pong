# Pong's tests. Gameplay tests are headless (label "cpu"); smoke tests launch
# the real game in GameLib's self-test mode (label "gpu").

add_executable(pong_game_tests test_match.cpp test_ai.cpp)
target_link_libraries(pong_game_tests PRIVATE pong_game GameLib::testing)
gamelib_target_warnings(pong_game_tests)
gamelib_copy_runtime(pong_game_tests)
add_test(NAME pong_game_tests COMMAND pong_game_tests)
set_tests_properties(pong_game_tests PROPERTIES LABELS cpu TIMEOUT 120)

add_executable(pong_app_tests test_app.cpp)
target_link_libraries(pong_app_tests PRIVATE pong_app GameLib::testing)
gamelib_target_warnings(pong_app_tests)
gamelib_copy_runtime(pong_app_tests)
add_test(NAME pong_app_tests COMMAND pong_app_tests)
set_tests_properties(pong_app_tests PROPERTIES LABELS cpu TIMEOUT 120)

# --- Smoke tests: the real game in GameLib's self-test mode ------------------
# Hidden window, validation layer REQUIRED (missing layer = failure), exit
# after N presented frames, any validation message = failure, no GPU = skipped
# (77). Settings go to the build tree (PONG_DATA_DIR), never the user's folder.
gamelib_validation_layer_dir(_layer_dir)
set(_smoke_env "PONG_DATA_DIR=set:${CMAKE_CURRENT_BINARY_DIR}/smoke_data")
if(_layer_dir)
    list(APPEND _smoke_env "VK_ADD_LAYER_PATH=path_list_prepend:${_layer_dir}")
endif()

function(pong_smoke_test name)
    add_test(NAME pong_smoke_${name}
             COMMAND pong --hidden --require-validation ${ARGN})
    set_tests_properties(pong_smoke_${name} PROPERTIES
        LABELS "gpu;smoke"
        TIMEOUT 120
        SKIP_RETURN_CODE 77
        ENVIRONMENT_MODIFICATION "${_smoke_env}")
endfunction()

# Title screen with the attract match behind the menu (vsync on: ~10 s of play).
pong_smoke_test(title --frames 600)
# A full computer-vs-computer match on the playing screen, unthrottled: many
# serves, rallies and points per second of wall time.
pong_smoke_test(demo --demo --no-vsync --frames 20000)
# Swapchain recreation while playing: a resize, then minimize and restore.
pong_smoke_test(resize_minimize --demo --frames 300 --resize-at-frame 60 --minimize-at-frame 150)
