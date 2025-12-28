add_library(game STATIC)

target_sources(game PRIVATE
  walker.c
  world.c
  utility.c
  simulation.c
)

target_compile_definitions(game PUBLIC _XOPEN_SOURCE=500)

target_compile_options(game PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -std=c11
)

target_include_directories(game PUBLIC ..)

# only if needed:
# target_link_libraries(randomWalk PUBLIC rt)
