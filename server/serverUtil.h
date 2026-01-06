#ifndef SERVER_UTIL_H
#define SERVER_UTIL_H
#include <pthread.h>
#include <game/simulation.h>
#include <ipc/ipcUtil.h>
#include <game/utility.h>

#define SERVER_CAPACITY 5

typedef enum {
  CLIENT_UNUSED = 0,
  CLIENT_ACTIVE = 1,
  CLIENT_TERMINATED = 2
} client_state_t;

typedef struct {
  summary_type_t sType;
  client_state_t state;
  ipc_ctx_t ipc;
  atomic_bool active;
  pthread_t tid;
  _Bool isAdmin;
} client_data_t;

typedef struct {
  client_data_t clients[SERVER_CAPACITY];
  int lastRemoved;
  int clientCount;
  int maxClients;
  _Bool adminSet;
  pthread_mutex_t cMutex;
  pthread_cond_t add;
} client_management_t;

typedef struct {
  viewmode_type_t viewMode;
  client_management_t cManagement;
  pthread_mutex_t viewMutex;
  pthread_mutex_t simMutex;
  simulation_t* sim;
  atomic_bool* running;
  ipc_ctx_t* ipc;
} server_ctx_t;

typedef struct {
  server_ctx_t* ctx;
  int clientPos;
} recv_data_t;


int add_client(client_management_t* mng, client_data_t c);
void remove_client(client_management_t* mng, int pos);

int server_ctx_init(server_ctx_t* ctx, simulation_t* sim, atomic_bool* running, ipc_ctx_t* ipc, _Bool moreClients);
void server_ctx_destroy(server_ctx_t* ctx);

#endif
