add_executable(server)

target_sources(server PRIVATE
  clientConnectionManager.c
  clientSession.c
  ipcContext.c
  server.c
)

target_compile_options(server PUBLIC
  -Wall
  -Wextra
  -Wpedantic
  -std=c11
)

target_link_libraries(server PUBLIC
  pthread
  game
)

target_include_directories(server PRIVATE
  ${CMAKE_SOURCE_DIR}
)
target_include_directories(game PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
