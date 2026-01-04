#include "ipcUtil.h"
#include "ipc/ipcPipe.h"
#include "ipc/ipcShmSem.h"
#include "ipc/ipcSocket.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int ipc_init(ipc_ctx_t* ipc, int who, const char* type, int port) {
  if (!ipc) return 1;
  memset(ipc, 0, sizeof(ipc_ctx_t));
  if (strcmp(type, "sock") == 0) {
    socket_t s;
    if (who == 0) {
      s = socket_init_server(port);
    } else {
      s = socket_init_client("127.0.0.1", port);
    }
    if (s.fd <= 0) {
      perror("Crating connection failed\n");
      return 1;
    }
    ipc->sock = malloc(sizeof(socket_t));
    if (!ipc->sock) {
      return 1;
    }
    *ipc->sock = s;
    ipc->type = 2;
  } else if (strcmp(type, "pipe") == 0) {
    //TODO
    return 1;
  } else if (strcmp(type, "shm") == 0) {
    //TODO
    return 1;
  }

  return 0;
}

int ipc_destroy(ipc_ctx_t* ipc) {
  if (!ipc) return 1;
  switch (ipc->type) {
    case 0:
      pipe_close(ipc->pipe);
      free(ipc->pipe);
      ipc->pipe = NULL;
      break;
    case 1:
      shm_close(ipc->shm);
      free(ipc->shm);
      ipc->shm = NULL;
      break;
    case 2:
      socket_shutdown(ipc->sock);
      socket_close(ipc->sock);
      free(ipc->sock);
      ipc->sock = NULL;
      break;
    default:
      return 1;
  };

  return 0;
}
