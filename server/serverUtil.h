#ifndef SERVER_UTIL_H
#define SERVER_UTIL_H
#include <pthread.h>
#include <game/simulation.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <ipc/ipcSocket.h>

typedef struct {
    int type; // 0=pipe,1=shm,2=sock
    pipe_t* pipe;
    shm_t* shm;
    socket_t* sock;
    simulation_t* sim;
    atomic_bool* running;
} server_ctx_t;

typedef struct {
    int cur, total, w, h;
} packet_header_t;

#endif
