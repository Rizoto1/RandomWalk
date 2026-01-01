#include "serverUtil.h"
#include <string.h>
#include <ipc/ipcSocket.h>

void add_client(client_management_t* mng, client_data_t c) {
  if (mng->lastRemoved < 0 && mng->clientCount < SERVER_CAPACITY) {
    mng->clients[mng->clientCount] = c;
    mng->clientCount++;
  } else if (mng->clientCount < SERVER_CAPACITY) {
    mng->clients[mng->lastRemoved] = c;
    mng->clientCount++;
  }
}

void remove_client(client_management_t* mng, int pos) {
  socket_close(&mng->clients[pos].socket);
  memset(&mng->clients[pos], 0, sizeof(client_data_t));
  mng->lastRemoved = pos;
}
