#ifndef SERVER_H
#define SERVER_H

#include "clientConnectionManager.h"
#include "clientSession.h"
#include <stdatomic.h>

typedef struct {
    client_connection_manager_t conn_mgr;
    atomic_bool running;
} server_t;

_Bool server_start(server_t* this, int port);
void server_stop(server_t* this);
void server_run(server_t* this);
void server_broadcast(server_t* this, const void* data, size_t size);
void server_handle_request(server_t* this, client_session_t* session, const void* data, size_t size);

#endif

