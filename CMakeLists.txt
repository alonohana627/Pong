# Pong - the classic, rebuilt on GameLib: procedural Vulkan visuals,
# synthesized sounds, a beatable computer opponent.
cmake_minimum_required(VERSION 3.28)

# C is enabled too because GameLib's dependencies (GLFW, miniaudio) are C.
project(Pong VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(PONG_BUILD_TESTS "Build Pong's tests" ON)

# --- GameLib -----------------------------------------------------------------
# GameLib is consumed as a library target, never copied. By default it is the
# sibling checkout next to this folder (../GameLib). To use another checkout,
# configure with -DFETCHCONTENT_SOURCE_DIR_GAMELIB=<path> (best kept in
# CMakeUserPresets.json). Once GameLib has a remote, replace SOURCE_DIR with
# GIT_REPOSITORY + a pinned GIT_TAG commit.
include(FetchContent)
FetchContent_Declare(GameLib SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../GameLib")
FetchContent_MakeAvailable(GameLib)

# Print the GameLib revision being built against, for bug reports and for the
# known-good combination recorded in docs/GAMELIB_REVISION.md.
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${gamelib_SOURCE_DIR}" rev-parse --short HEAD
                    OUTPUT_VARIABLE _gamelib_revision OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    message(STATUS "Pong: using GameLib ${_gamelib_revision} from ${gamelib_SOURCE_DIR}")
endif()

add_subdirectory(src)

if(PONG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
