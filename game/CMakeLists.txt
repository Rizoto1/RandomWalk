add_library(randomWalk SHARED)

target_sources(randomWalk PRIVATE
walker.c
world.c
)

target_compile_definitions(randomWalk PUBLIC _XOPEN_SOURCE=500)

target_compile_options(randomWalk PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -std=c11
)

target_include_directories(randomWalk PUBLIC ..)
target_link_libraries(randomWalk PUBLIC rt)
