#ifndef CLIENT_UTIL_H
#define CLIENT_UTIL_H

#include <stdatomic.h>
#include <ipc/ipcUtil.h>

typedef struct {
    ipc_ctx_t* ipc;
    atomic_bool running;
} client_context_t;

void clear_screen(void);

void ctx_destroy(client_context_t* ctx);
int ctx_init(client_context_t* ctx, ipc_ctx_t* ipc);

#endif
