#ifndef IPC_UTIL_H
#define IPC_UTIL_H
#include "ipcPipe.h"
#include "ipcShmSem.h"
#include "ipcSocket.h"

typedef struct {
  int type; // 0=pipe,1=shm,2=sock
  pipe_t* pipe;
  shm_t* shm;
  socket_t* sock;
} ipc_ctx_t;

int ipc_init(ipc_ctx_t* ipc, int who, const char* type, int port);
int ipc_destroy(ipc_ctx_t* ipc);

#endif
