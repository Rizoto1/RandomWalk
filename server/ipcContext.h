#ifndef IPC_CONTEXT_H
#define IPC_CONTEXT_H

#include <stddef.h>

typedef enum {
    IPC_PIPE,
    IPC_SEMAPHORE,
    IPC_SOCKET
} ipc_type_t;

typedef struct {
    int pipe_in;
    int pipe_out;
    int shm_id;
    int socket_id;
    ipc_type_t type;
} ipc_context_t;

_Bool ipc_init(ipc_context_t* this, ipc_type_t type);
int ipc_send(ipc_context_t* this, const void* data, size_t size);
int ipc_receive(ipc_context_t* this, void* buffer, size_t size);
void ipc_close(ipc_context_t* this);
_Bool ipc_is_valid(ipc_context_t* this);
int ipc_get_fd(ipc_context_t* this);

#endif

