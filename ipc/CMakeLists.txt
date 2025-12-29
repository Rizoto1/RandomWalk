add_library(ipc STATIC)

target_sources(ipc PRIVATE
  ipcPipe.c
  ipcSocket.c
  ipcShmSem.c
)

target_compile_definitions(ipc PUBLIC _XOPEN_SOURCE=500)

target_compile_options(ipc PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -std=c11
)

target_include_directories(ipc PUBLIC ..)

# only if needed:
# target_link_libraries(randomWalk PUBLIC rt)
