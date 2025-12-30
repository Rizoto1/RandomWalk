#ifndef CLIENT_UTIL_H
#define CLIENT_UTIL_H

#include <stdatomic.h>
#include <ipc/ipcSocket.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <stdio.h>

typedef struct {
    int type; // 0=pipe,1=shm,2=sock
    pipe_t* pipe;
    shm_t* shm;
    socket_t* socket;
    atomic_bool running;
} client_context_t;

typedef struct {
  int cur, total, w, h;
} packet_header_t;

void clear_screen() {
  printf("\033[2J\033[1;1H");
}
#endif
