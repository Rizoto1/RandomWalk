#include "serverUtil.h"
#include <ipc/ipcSocket.h>
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
    if (mng->clients[pos].state == CLIENT_ACTIVE) {
        socket_shutdown(&mng->clients[pos].socket);
        socket_close(&mng->clients[pos].socket);

        mng->clients[pos].state = CLIENT_UNUSED;
        atomic_store(&mng->clients[pos].active, 0);
        mng->clients[pos].tid = 0;

        mng->clientCount--;
    }
}

