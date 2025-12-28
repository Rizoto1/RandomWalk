#include "server.h"
#include <game/simulation.h>
#include <game/walker.h>
#include <game/world.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void* simulation_thread(void* arg) {
  server_t* this = arg;

  sim_run(&this->simulation, &this->running);

  return NULL;
}

_Bool server_init(server_t* this, int port,
                  int world_w, int world_h,
                  int reps, int k,
                  double up, double down,
                  double right, double left, const char* fPath)
{
  memset(this, 0, sizeof(*this));
  this->port = port;
  atomic_store(&this->running, 0);

  // init simulation
  if (!simulation_init(&this->simulation, world_w, world_h, reps)) {
    fprintf(stderr, "[server] simulation_init failed\n");
    return 0;
  }

  // init connection manager (socket only variant)
  if (!client_connection_manager_init(&this->conn_mgr, port)) {
    fprintf(stderr, "[server] conn_mgr init failed!\n");
    return 0;
  }

  printf("[server] initialized on port %d\n", port);
  return 1;
}

void server_run(server_t* this)
{
  atomic_store(&this->running, 1);

  // spustíme thread pre simuláciu
  pthread_create(&this->sim_thread, NULL, simulation_thread, this);

  printf("[server] listening for client sessions...\n");

  // hlavná slučka
  while (atomic_load(&this->running))
  {
    client_session_t* session = NULL;

    // blokuje kým nepríde žiadosť
    if (!client_connection_manager_wait_for_session(&this->conn_mgr, &session))
      continue;

    // dostali sme dáta od klienta
    char buffer[256];
    int n = client_session_receive(session, buffer, sizeof(buffer));
    if (n <= 0) {
      client_session_close(session);
      continue;
    }

    server_handle_request(this, session, buffer, (size_t)n);
  }
}

void server_stop(server_t* this)
{
  atomic_store(&this->running, 0);

  pthread_join(this->sim_thread, NULL);
  client_connection_manager_shutdown(&this->conn_mgr);
  simulation_destroy(&this->simulation);

  printf("[server] stopped\n");
}

void server_broadcast(server_t* this, const void* data, size_t size)
{
  // iterácia cez sessions (dočasný minimalistický model)
  for (int i = 0; i < this->conn_mgr.count; i++) {
    client_session_t* sess = this->conn_mgr.sessions[i];
    if (sess && sess->connected) {
      client_session_send(sess, data, size, NULL);
    }
  }
}

void server_handle_request(server_t* this, client_session_t* session,
                           const void* data, size_t size)
{
  // primitívny protokol
  if (strncmp((const char*)data, "state", 5) == 0) {
    char world_dump[2048];
    simulation_dump(&this->simulation, world_dump, sizeof(world_dump));
    client_session_send(session, world_dump, strlen(world_dump), NULL);
  }
  else if (strncmp((const char*)data, "step", 4) == 0) {
    simulation_step(&this->simulation);
    client_session_send(session, "OK\n", 3, NULL);
  }
  else if (strncmp((const char*)data, "quit", 4) == 0) {
    client_session_close(session);
    printf("[server] client exited.\n");
  }
  else {
    const char* err = "ERR: unknown command\n";
    client_session_send(session, err, strlen(err), NULL);
  }
}

