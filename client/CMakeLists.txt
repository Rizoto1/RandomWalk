add_executable(client)

target_sources(client PRIVATE
  client.c
)

target_compile_options(client PUBLIC
  -Wall
  -Wextra
  -Wpedantic
  -std=c11
)

target_link_libraries(client PUBLIC
  pthread
  game
  ipc
)

target_include_directories(client PRIVATE
  ${CMAKE_SOURCE_DIR}
)
target_include_directories(game PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
