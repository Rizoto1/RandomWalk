#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include <stddef.h>
#include "ipcContext.h"

typedef struct {
    ipc_context_t ctx;
    int id;
    _Bool connected;
} client_session_t;

_Bool client_session_init(client_session_t* this, int id, ipc_context_t* ctx);
int client_session_send(client_session_t* this, const void* data, size_t size);
int client_session_receive(client_session_t* this, void* buffer, size_t size);
void client_session_close(client_session_t* this);
int client_session_fd(client_session_t* this);

#endif

