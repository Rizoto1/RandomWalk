#ifndef CLIENT_CONNECTION_MANAGER_H
#define CLIENT_CONNECTION_MANAGER_H

#include "clientSession.h"

#define MAX_CLIENTS 64  /* optional */

typedef struct {
    client_session_t sessions[MAX_CLIENTS];
    int session_count;
    int listen_fd;
} client_connection_manager_t;

_Bool conn_manager_init(client_connection_manager_t* this, int port);
client_session_t* conn_manager_accept(client_connection_manager_t* this);
void conn_manager_remove(client_connection_manager_t* this, client_session_t* session);
void conn_manager_poll(client_connection_manager_t* this);
int conn_manager_get_count(client_connection_manager_t* this);
client_session_t* conn_manager_get_session(client_connection_manager_t* this, int index);

#endif

