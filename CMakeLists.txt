cmake_minimum_required(VERSION 3.5.0)
project(MazeRunner VERSION 0.1.0 LANGUAGES C CXX)

add_compile_options(-Wno-narrowing)

find_package(nlohmann_json 3.11.3 REQUIRED)

# Too lazy to install raylib to the system.
include(FetchContent)
set(RAYLIB_VERSION 5.0)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS ${RAYLIB_VERSION}
)
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(OPENGL_VERSION "4.3")
FetchContent_MakeAvailable(raylib)

file(GLOB SOURCES Source/*.cpp Source/*.h)

add_executable(MazeRunner ${SOURCES})
message(STATUS "\n<3 here is sources: ${SOURCES}\n")

target_link_libraries(MazeRunner PRIVATE nlohmann_json::nlohmann_json raylib rlImGui)
