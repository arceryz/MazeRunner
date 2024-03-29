cmake_minimum_required(VERSION 3.5.0)
project(MazeRunner VERSION 0.1.0 LANGUAGES C CXX)

find_package(nlohmann_json 3.11.3 REQUIRED)
find_package(raylib REQUIRED)

file(GLOB SOURCES *.cpp *.h)

add_executable(MazeRunner ${SOURCES})
message(STATUS "\n<3 here is sources: ${SOURCES}\n")

target_link_libraries(MazeRunner PRIVATE nlohmann_json::nlohmann_json raylib rlImGui)
