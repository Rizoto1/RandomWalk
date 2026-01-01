#ifndef SERVER_UTIL_H
#define SERVER_UTIL_H
#include <pthread.h>
#include <game/simulation.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <ipc/ipcSocket.h>
#include <game/utility.h>

#define SERVER_CAPACITY 5

typedef struct {
  summary_type_t sType;
  int id;
  socket_t socket;
  _Bool active;
} client_data_t;

typedef struct {
  client_data_t clients[SERVER_CAPACITY];
  int lastRemoved;
  int clientCount;
  pthread_mutex_t cMutex;
  pthread_cond_t add;
  pthread_cond_t remove;
  int idCounter;
} client_management_t;

typedef struct {
  int type; // 0=pipe,1=shm,2=sock
  viewmode_type_t viewMode;
  client_management_t cManagement;
  pthread_mutex_t vMutex;
  pipe_t* pipe;
  shm_t* shm;
  socket_t* sock;
  simulation_t* sim;
  atomic_bool* running;
} server_ctx_t;

typedef struct {
  server_ctx_t* ctx;
  int clientPos;
} recv_data_t;

typedef struct {
  int cur, total, w, h;
} packet_header_t;

void add_client(client_management_t* mng, client_data_t c);
void remove_client(client_management_t* mng, int pos);

#endif
