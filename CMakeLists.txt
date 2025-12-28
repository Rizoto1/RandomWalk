cmake_minimum_required(VERSION 3.22)

project(
  RandomWalk
  LANGUAGES C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(game)
add_subdirectory(server)
