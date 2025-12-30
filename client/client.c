#include "clientUtil.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ipc/ipcSocket.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <game/simulation.h>
/**
 * Receive thread - continuously receives packets from server
 * Packet:
 * Header + (w*h*2 doubles)
 */
void* client_thread_receive(void* arg) {
    client_context_t* ctx = (client_context_t*)arg;

    while (atomic_load(ctx->running)) {
        packet_header_t hdr;
        int r = socket_recv(ctx->socket, &hdr, sizeof(hdr));
        if (r <= 0) break;

        int count = hdr.w * hdr.h * 2;
        double* buffer = malloc(count * sizeof(double));
        socket_recv(ctx->socket, buffer, count * sizeof(double));

        printf("\nReplication %d / %d\n", hdr.cur, hdr.total);
        for (int i = 0; i < hdr.h; i++) {
            for (int j = 0; j < hdr.w; j++) {
                int idx = (i * hdr.w + j) * 2;
                printf("(%.4f , %.4f) ", buffer[idx], buffer[idx + 1]);
            }
            printf("\n");
        }

        free(buffer);
    }
    return NULL;
}


/**
 * Send thread - reads commands from keyboard and sends to server
 * Commands:
 * 's' - interactive mode on
 * 'u' - summary mode
 * 'q' - request stop
 */
void* client_thread_send(void* arg) {
    client_context_t* ctx = (client_context_t*)arg;

    while (atomic_load(ctx->running)) {
        char c = getchar();
        if (c == '\n') continue;

        socket_send(ctx->socket, &c, sizeof(c));

        if (c == 'q') {         // user requests quit
            atomic_store(ctx->running, 0);
        }
    }
    return NULL;
}

void newGame() {

}

void connectToGame() {

}

void continueInGame() {

}

void end() {

}

void mainMenu() {
  return;
}


/**
 * MAIN - initializes client and launches send + receive threads
 */
int main(int argc, char** argv) {
    /*const char* ip = "127.0.0.1";
    int port = 7777;

    if (argc >= 2) ip   = argv[1];
    if (argc >= 3) port = atoi(argv[2]);

    atomic_bool running;
    atomic_init(&running, 1);

    // create socket connection
    socket_t sock = socket_init_client(ip, port);

    // build context
    client_context_t ctx = { 3, NULL, NULL, &sock, &running };

    pthread_t tRecv, tSend;
    pthread_create(&tRecv, NULL, client_thread_receive, &ctx);
    pthread_create(&tSend, NULL, client_thread_send, &ctx);

    // wait for exit
    pthread_join(tSend, NULL);
    pthread_join(tRecv, NULL);

    socket_close(&sock);*/
    return 0;
}

