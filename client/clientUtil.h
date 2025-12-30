#ifndef CLIENT_UTIL_H
#define CLIENT_UTIL_H

#include <stdatomic.h>
#include <ipc/ipcSocket.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>

typedef struct {
    int type;
    pipe_t* pipe;
    shm_t* shm;
    socket_t* socket;
    atomic_bool* running;
} client_context_t;

typedef struct {
  int cur, total, w, h;
} packet_header_t;

#endif
