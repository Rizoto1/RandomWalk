#include "serverUtil.h"
#include "game/utility.h"
#include "ipc/ipcUtil.h"
#include <ipc/ipcSocket.h>
#include <game/simulation.h>
#include <pthread.h>
#include <stdatomic.h>

int add_client(client_management_t* mng, client_data_t c) {
    for (int i = 0; i < SERVER_CAPACITY; i++) {
        if (mng->clients[i].state == CLIENT_UNUSED) {
            mng->clients[i] = c;
            mng->clients[i].state = CLIENT_ACTIVE;
            mng->clientCount++;
            return i; // index klienta
        }
    }
    return -1; // plné
}
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

int server_ctx_init(server_ctx_t* ctx, simulation_t* sim, atomic_bool* running, ipc_ctx_t* ipc) {
  if (!ctx || !sim || !running || !ipc) return 1;
  
  ctx->running = running;
  ctx->sim = sim;
  ctx->viewMode = SUMMARY;
  ctx->ipc = ipc;
  pthread_mutex_init(&ctx->viewMutex, NULL);
  pthread_mutex_init(&ctx->cManagement.cMutex, NULL);
  pthread_mutex_init(&ctx->simMutex, NULL);
  pthread_cond_init(&ctx->cManagement.add, NULL);
  pthread_cond_init(&ctx->cManagement.remove, NULL);

  return 0;
}
