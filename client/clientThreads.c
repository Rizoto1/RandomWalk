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
#include "ui.h"
#include <math.h>

static void recv_summary(client_context_t* ctx, packet_header_t* hdr) {
  int count = hdr->w * hdr->h;
  double* buffer = malloc(count * sizeof(double));
  socket_recv(ctx->socket, buffer, count * sizeof(double));

  printf("\nReplication %d / %d\n", hdr->cur + 1, hdr->total);
  for (int i = 0; i < hdr->h; i++) {
    for (int j = 0; j < hdr->w; j++) {
      int idx = (i * hdr->w + j);
      printf("%.2f ", buffer[idx]);
    }
    printf("\n");
  }

  free(buffer);
}

static void recv_interactive(client_context_t* ctx, packet_header_t* hdr) {
  int count = hdr->w * hdr->h;
  char* buffer = malloc(count * sizeof(char));
  socket_recv(ctx->socket, buffer, count * sizeof(char));

  int k = hdr->k;
  position_t* posBuf = malloc(k * sizeof(position_t));
  socket_recv(ctx->socket, posBuf, k * sizeof(position_t));
  
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
    int r = socket_recv(ctx->socket, &hdr, sizeof(hdr));
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

    int r = socket_send(ctx->socket, &c, sizeof(c));

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

  printf("\nSimulacia bezi. Piste prikazy:\n");
  printf("napr. 'STOP' ukonci spojenie\n\n");

  // Wait — when user types STOP → terminate
  pthread_join(send_th, NULL);

  context->running = 0;
  pthread_cancel(recv_th);
  pthread_join(recv_th, NULL);
  if (context->type == 0) {
    pipe_close(context->pipe);
  } else if (context->type == 1) { 
    shm_close(context->shm); 
  } else if (context->type == 2) {
    socket_close(context->socket);
  } else {
    perror("Simulation menu: invalid ipc type");
    return;
  }
}
void createServer(int type,
                  double up, double down, double right, double left,
                  int width, int height, world_type_t worldType, int obstaclePercentage,
                  viewmode_type_t viewMode,
                  int replications, int k, char* savePath) {
  pid_t pid = fork();
  char portBuf[16], upBuf[16], downBuf[16], rightBuf[16], leftBuf[16], ipcBuf[16], wTypeBuf[16];
  char widthBuf[16], heightBuf[16], obstBuf[16], replBuf[16], kBuf[16], vModeBuf[16];

  /* sprintf alebo bezpečnejší snprintf */
  snprintf(ipcBuf, sizeof(ipcBuf), "%d", type);
  snprintf(portBuf, sizeof(portBuf), "%d", PORT);
  snprintf(upBuf, sizeof(upBuf), "%lf", up);
  snprintf(downBuf, sizeof(downBuf), "%lf", down);
  snprintf(rightBuf, sizeof(rightBuf), "%lf", right);
  snprintf(leftBuf, sizeof(leftBuf), "%lf", left);
  snprintf(widthBuf, sizeof(widthBuf), "%d", width);
  snprintf(heightBuf, sizeof(heightBuf), "%d", height);
  snprintf(wTypeBuf, sizeof(wTypeBuf), "%d", worldType);
  snprintf(obstBuf, sizeof(obstBuf), "%d", obstaclePercentage);
  snprintf(vModeBuf, sizeof(vModeBuf), "%d", viewMode);
  snprintf(replBuf, sizeof(replBuf), "%d", replications);
  snprintf(kBuf, sizeof(kBuf), "%d", k);

  sleep(5);

  if (pid == 0) {
    // child → exec server
    char *args[] = {
      "./server",
      ipcBuf,
      portBuf,
      upBuf,
      downBuf,
      rightBuf,
      leftBuf,
      widthBuf,
      heightBuf,
      wTypeBuf,
      obstBuf,
      vModeBuf,
      replBuf,
      kBuf,
      savePath,
      NULL     // koniec pre exec()
    };

    execv("./server", args);
    perror("exec");
    exit(1);
  }

  client_context_t ctx;
  ctx_init(&ctx);
  ctx.type = type;
  socket_t s = socket_init_client("127.0.0.1", PORT);
  ctx.socket = &s;
  /*if (type == 0)
    //ipc_init_pipe(&ctx, DEFAULT_PIPE_NAME);
  else if (type == 2)
    ipc_init_socket(&ctx, DEFAULT_SOCKET_PORT);
  else
    //ipc_init_shared_memory(&ctx, DEFAULT_SHM_KEY, 1024);*/

  simulation_menu(&ctx);
  ctx_destroy(&ctx);
}
