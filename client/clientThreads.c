#include "clientThreads.h"
#include "clientUtil.h"
#include <game/utility.h>
#include <game/world.h>
#include <game/walker.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ipc/ipcSocket.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <game/simulation.h>
#include <game/walker.h>
#include "ipc/ipcUtil.h"
#include "ui.h"

static void recv_summary(client_context_t* ctx, packet_header_t* hdr) {
  int count = hdr->w * hdr->h;
  double* buffer = malloc(count * sizeof(double));
  socket_recv(&ctx->ipc->sock, buffer, count * sizeof(double));

  printf("\nReplication %d / %d\n", hdr->cur, hdr->total);
  for (int i = 0; i < hdr->h; i++) {
    for (int j = 0; j < hdr->w; j++) {
      int idx = (i * hdr->w + j);
      if (buffer[idx] == -1) {
        printf("  #  ");
        continue;
      }
      printf("%.2f ", buffer[idx]);
    }
    printf("\n");
  }

  free(buffer);
}

static void recv_interactive(client_context_t* ctx, packet_header_t* hdr) {
  int count = hdr->w * hdr->h;
  char* buffer = malloc(count * sizeof(char));
  socket_recv(&ctx->ipc->sock, buffer, count * sizeof(char));

  int k = hdr->k;
  position_t* posBuf = malloc(k * sizeof(position_t));
  socket_recv(&ctx->ipc->sock, posBuf, k * sizeof(position_t));
  
  draw_interactive_map(buffer, posBuf, hdr);

  free(buffer);
  free(posBuf);
}

/**
 * Receive thread - continuously receives packets from server
 * Packet:
 * Header + (w*h*2 doubles)
 */
void* thread_receive(void* arg) {
  client_context_t* ctx = (client_context_t*)arg;
  printf("Clinet: Starting recv thread.\n");

  while (atomic_load(&ctx->running)) {
    clear_screen();
    packet_header_t hdr;
    int r = socket_recv(&ctx->ipc->sock, &hdr, sizeof(hdr));
    if (r <= 0) {
      printf("Client: Terminating recv thread. Opposite site closed connection\n");
      atomic_store(&ctx->running, 0);
      return NULL;
    }


    if (hdr.type == PKT_SUMMARY) {
      recv_summary(ctx, &hdr);
    } else {
      recv_interactive(ctx, &hdr);
    }
  }

  printf("Client: Terminating recv thread. \n");
  return NULL;
}

/**
 * Send thread - reads commands from keyboard and sends to server
 * Commands:
 * 's' - interactive mode on
 * 'u' - summary mode
 * 'q' - request stop
 */
void* thread_send(void* arg) {
  client_context_t* ctx = (client_context_t*)arg;
  printf("Client: Starting send thread.\n");

  while (atomic_load(&ctx->running)) {
    char c = getchar();
    if (c == '\n') continue;

    int r = socket_send(&ctx->ipc->sock, &c, sizeof(c));

    if (r <= 0) {
      atomic_store(&ctx->running, 0);
      break;
    }

    if (c == 'q') {         // user requests quit
      atomic_store(&ctx->running, 0);
      break;
    }
  }
  printf("Client: Terminating send thread.\n");
  return NULL;
}

void simulation_menu(client_context_t* context) {
  pthread_t recv_th, send_th;
  context->running = 1;

  pthread_create(&recv_th, NULL, thread_receive, context);
  pthread_create(&send_th, NULL, thread_send, context);

  sleep(2);
  // Wait — when user types STOP → terminate
  pthread_join(send_th, NULL);
  
  atomic_store(&context->running, 0);

  pthread_cancel(recv_th);
  pthread_join(recv_th, NULL);

  ipc_destroy(context->ipc);

}

void createServer(int type, int port,
                  double up, double down, double right, double left,
                  int width, int height, world_type_t worldType, int obstaclePercentage,
                  int serverLoadType,
                  int replications, int k, char* savePath) {
  pid_t pid = fork();
  char portBuf[16], upBuf[16], downBuf[16], rightBuf[16], leftBuf[16], ipcBuf[16], wTypeBuf[16];
  char widthBuf[16], heightBuf[16], obstBuf[16], replBuf[16], kBuf[16], serModeBuf[16];

  /* sprintf alebo bezpečnejší snprintf */
  snprintf(ipcBuf, sizeof(ipcBuf), "%d", type);
  snprintf(portBuf, sizeof(portBuf), "%d", port);
  snprintf(upBuf, sizeof(upBuf), "%lf", up);
  snprintf(downBuf, sizeof(downBuf), "%lf", down);
  snprintf(rightBuf, sizeof(rightBuf), "%lf", right);
  snprintf(leftBuf, sizeof(leftBuf), "%lf", left);
  snprintf(widthBuf, sizeof(widthBuf), "%d", width);
  snprintf(heightBuf, sizeof(heightBuf), "%d", height);
  snprintf(wTypeBuf, sizeof(wTypeBuf), "%d", worldType);
  snprintf(obstBuf, sizeof(obstBuf), "%d", obstaclePercentage);
  snprintf(serModeBuf, sizeof(serModeBuf), "%d", serverLoadType);
  snprintf(replBuf, sizeof(replBuf), "%d", replications);
  snprintf(kBuf, sizeof(kBuf), "%d", k);

  if (pid < 0) {
    perror("fork not forking");
    exit(1);
}
  sleep(1);

  if (pid == 0) {
    // child → exec server
    char *args[] = {
      "server",
      serModeBuf,
      "sock",
      portBuf,
      upBuf,
      downBuf,
      rightBuf,
      leftBuf,
      widthBuf,
      heightBuf,
      wTypeBuf,
      obstBuf,
      replBuf,
      kBuf,
      savePath,
      NULL     // koniec pre exec()
    };
    execv("./server/server", args);
    perror("exec");
  }

  ipc_ctx_t ipc;
  if(ipc_init(&ipc, 1, "sock", port)) {
    perror("Client: Connection init failed\n");
    return;
  };
  client_context_t ctx;
  if(ctx_init(&ctx, &ipc)) {
    perror("Client: Client init failed\n");
    return;
  };
  sleep(5);

  simulation_menu(&ctx);
  ctx_destroy(&ctx);
}

void loadServer(int serverLoadType,
                int type, int port,
                int replications, char* loadPath, char* savePath) {
  pid_t pid = fork();
  char serModeBuf[16], ipcBuf[16], portBuf[16], replBuf[16];

  /* sprintf alebo bezpečnejší snprintf */
  snprintf(serModeBuf, sizeof(serModeBuf), "%d", serverLoadType);
  snprintf(ipcBuf, sizeof(ipcBuf), "%d", type);
  snprintf(portBuf, sizeof(portBuf), "%d", port);
  snprintf(replBuf, sizeof(replBuf), "%d", replications);

  if (pid < 0) {
    perror("fork not forking");
    exit(1);
}
  sleep(1);

  if (pid == 0) {
    // child → exec server
    char *args[] = {
      "server",
      serModeBuf,
      "sock",
      portBuf,
      loadPath,
      replBuf,
      savePath,
      NULL     // koniec pre exec()
    };
    execv("./server/server", args);
    perror("exec");
  }

  ipc_ctx_t ipc;
  if(ipc_init(&ipc, 1, "sock", port)) {
    perror("Client: Connection init failed\n");
    return;
  };
  client_context_t ctx;
  if(ctx_init(&ctx, &ipc)) {
    perror("Client: Client init failed\n");
    return;
  };
  sleep(5);

  simulation_menu(&ctx);
  ctx_destroy(&ctx);
}
