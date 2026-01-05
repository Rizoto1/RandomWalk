#include "serverUtil.h"
#include "game/utility.h"
#include "ipc/ipcUtil.h"
#include <ipc/ipcSocket.h>
#include <game/simulation.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>

/*
 * Add client to client management.
 * If addition is succesful return index of added user. Otherwise returns -1.
 */
int add_client(client_management_t* mng, client_data_t c) {
    for (int i = 0; i < SERVER_CAPACITY; i++) {
        if (mng->clients[i].state == CLIENT_UNUSED) {
            mng->clients[i] = c;
            mng->clients[i].state = CLIENT_ACTIVE;
            mng->clientCount++;
            return i;
        }
    }
    return -1;
}

/*
 * Removes client from client management based on pos.
 */
void remove_client(client_management_t* mng, int pos) {
    if (mng->clients[pos].state == CLIENT_ACTIVE || mng->clients[pos].state == CLIENT_TERMINATED) {
        ipc_destroy(&mng->clients[pos].ipc);

        mng->clients[pos].state = CLIENT_UNUSED;
        atomic_store(&mng->clients[pos].active, 0);
        mng->clients[pos].tid = 0;
        //mng->clients[pos].socket.fd = -1;

        mng->clientCount--;

        pthread_cond_signal(&mng->add);
    }
}

/*
 * Initializes server context.
 * If initialization succeeds return 0, otherwise 1.
 */
int server_ctx_init(server_ctx_t* ctx, simulation_t* sim, atomic_bool* running, ipc_ctx_t* ipc) {
  if (!ctx || !sim || !running || !ipc) return 1;
  memset(ctx, 0, sizeof(*ctx)); 
  ctx->running = running;
  ctx->sim = sim;
  ctx->viewMode = SUMMARY;
  ctx->ipc = ipc;
  ctx->cManagement.creatorPos = -1;
  ctx->cManagement.creatorSet = 0;
  pthread_mutex_init(&ctx->viewMutex, NULL);
  pthread_mutex_init(&ctx->cManagement.cMutex, NULL);
  pthread_mutex_init(&ctx->simMutex, NULL);
  pthread_cond_init(&ctx->cManagement.add, NULL);

  return 0;
}

/*
 * Destroys server context.
 */
void server_ctx_destroy(server_ctx_t* ctx) {
  if (!ctx) return;
  walker_destroy(&ctx->sim->walker);
  w_destroy(&ctx->sim->world);
  ipc_destroy(ctx->ipc);
  pthread_mutex_destroy(&ctx->viewMutex);
  pthread_mutex_destroy(&ctx->simMutex);
  pthread_mutex_destroy(&ctx->cManagement.cMutex);
  pthread_cond_destroy(&ctx->cManagement.add);
}
