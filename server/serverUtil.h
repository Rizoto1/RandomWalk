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
} ServerCtx;

#define MSG_STOP "STOP"
#define MSG_GET  "GET"
#define MSG_INTER "INTER"
#define MSG_SUM "SUM"

#endif
