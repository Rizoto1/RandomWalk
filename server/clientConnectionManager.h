#ifndef CLIENT_CONNECTION_MANAGER_H
#define CLIENT_CONNECTION_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include "clientSession.h"

#define MAX_CLIENTS 32

typedef struct {
    int listen_fd;
    client_session_t sessions[MAX_CLIENTS];
    int count;
} client_connection_manager_t;

bool conn_manager_init(client_connection_manager_t* this, int port);
client_session_t* conn_manager_accept(client_connection_manager_t* this);
void conn_manager_poll(client_connection_manager_t* this);
int  conn_manager_get_count(client_connection_manager_t* this);
client_session_t* conn_manager_get_session(client_connection_manager_t* this, int index);
void conn_manager_broadcast(client_connection_manager_t* this, const void* data, size_t len);
void conn_manager_close(client_connection_manager_t* this);

#endif

