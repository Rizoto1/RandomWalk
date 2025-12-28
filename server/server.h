#ifndef SERVER_H
#define SERVER_H

#include <stdatomic.h>
#include <pthread.h>

#include "clientConnectionManager.h"
#include "clientSession.h"
#include <game/simulation.h>

typedef struct server_s {
  client_connection_manager_t conn_mgr;
  simulation_t simulation;
  pthread_t sim_thread;
  atomic_bool running;
  int port;
} server_t;

_Bool server_init(server_t* this, int port,
                  int world_w, int world_h,
                  int reps, int k,
                  double up, double down,
                  double right, double left, const char* fPath);
void server_run(server_t* this);
void server_stop(server_t* this);
void server_broadcast(server_t* this, const void* data, size_t size);
void server_handle_request(server_t* this, client_session_t* session, const void* data, size_t size);

#endif

