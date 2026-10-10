add_subdirectory(game)
add_subdirectory(app)

# The executable is only main.cpp; everything else lives in the libraries.
add_executable(pong main.cpp)
target_link_libraries(pong PRIVATE pong_app)
gamelib_target_warnings(pong)
gamelib_copy_runtime(pong)
