#ifndef CLIENT_UTIL_H
#define CLIENT_UTIL_H

#include <stdatomic.h>
#include <ipc/ipcSocket.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <stdio.h>

#define PORT 6666

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

void clear_screen();

void ctx_destroy(client_context_t* ctx);
void ctx_init(client_context_t* ctx);

#endif
