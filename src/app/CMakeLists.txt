# pong_app: everything around the rules - menus, settings, sounds, effects,
# views and the GameLib main-loop glue. A library so its logic is testable.
add_library(pong_app STATIC
    effects.cpp
    menu.cpp
    pong_game.cpp
    settings.cpp
    sounds.cpp
    view.cpp)
target_include_directories(pong_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pong_app PUBLIC pong_game GameLib::app GameLib::storage)
gamelib_target_warnings(pong_app)
# Pong's own procedural shapes, embedded as SPIR-V (see view.hpp, Shapes).
gamelib_add_shaders(pong_app SHADERS
    ${PROJECT_SOURCE_DIR}/shaders/court.frag
    ${PROJECT_SOURCE_DIR}/shaders/digit.frag)
