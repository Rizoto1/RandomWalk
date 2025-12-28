#ifndef IPC_CONTEXT_H
#define IPC_CONTEXT_H

#include <stddef.h>
#include <pthread.h>
#include <semaphore.h>

typedef enum {
  IPC_PIPE,
  IPC_SEMAPHORE,
  IPC_SOCKET
} ipc_type_t;

typedef struct {
  ipc_type_t type;

  int listen_fd;
  int socket_fd;

  int pipefd[2];   // pipefd[0] = read, pipefd[1] = write

  int shm_id;
  void* shm_ptr;
  sem_t* sem;
  size_t shm_size;
} ipc_context_t;

_Bool ipc_init_socket(ipc_context_t* this, int port);
_Bool ipc_init_pipe(ipc_context_t* this);
_Bool ipc_init_semaphore(ipc_context_t* this, const char* name, size_t size);
_Bool ipc_init_from_fd(ipc_context_t* this, int fd);

int  ipc_send(ipc_context_t* this, const void* data, size_t size);
int  ipc_receive(ipc_context_t* this, void* buffer, size_t size);

int  ipc_accept_socket(ipc_context_t* this);
int  ipc_fd(ipc_context_t* this);
void ipc_close(ipc_context_t* this);

#endif

