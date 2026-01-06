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
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

/*
 * If the view mode is set to summary this function prints summary related data
 * sent by server.
 *
 * Made with help from AI.
 */
static void recv_summary(client_context_t* ctx, packet_header_t* hdr) {
  int count = hdr->w * hdr->h;
  double* buffer = malloc(count * sizeof(double));
  socket_recv(&ctx->ipc->sock, buffer, count * sizeof(double));

  draw_summary_map(buffer, hdr);

  free(buffer);
}

/*
 * If the view mode is set to interactive this function prints interactive related data
 * sent by server.
 *
 * Made with help from AI.
 */
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

/*
 * Continuously received data from server and displayes them on screen
 * using helper functions based on set view mode.
 *
 * Made with help from AI.
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

/*
 * Signal handler for interrupt blocking calls (e.g. getchar)
 *
 * Made with help from AI.
 */
static void wake_handler(int sig) {
  (void)sig;
}

/*
 * Sends users inputs to server.
 * s - summary - statistics
 * i - interactive - walkers trajectory
 * q - client disconnects from server
 * f - admin only (first user that joins server)
 *   - shutsdown server
 * a - summary 1 - displays average steps walker needs to do to reach center
 * b - summary 2 - displays probability to reach center.
 *
 * Made with help from AI.
 */
void* thread_send(void* arg) {
  client_context_t* ctx = (client_context_t*)arg;
  printf("Client: Starting send thread.\n");

  struct sigaction sa = {0};
  sa.sa_handler = wake_handler;
  sigaction(SIGUSR1, &sa, NULL);

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

/*
 * This function starts all threads and joins them as well.
 */
void simulation_menu(client_context_t* context) {
  pthread_t recv_th, send_th;
  context->running = 1;

  pthread_create(&recv_th, NULL, thread_receive, context);
  pthread_create(&send_th, NULL, thread_send, context);

  sleep(2);
  pthread_join(recv_th, NULL);
  pthread_kill(send_th, SIGUSR1);
  pthread_join(send_th, NULL);

  ipc_destroy(context->ipc);

}

/*
 * Creates server based on all the arguments. Then converts all non char types to char*.
 * Then forks and execv to create new process server with all the char* params.
 *
 * Made with help from AI.
 */
void create_server(int type, int port,
                   double up, double down, double right, double left,
                   int width, int height, world_type_t worldType, int obstaclePercentage,
                   int serverLoadType,
                   int replications, int k, char* savePath) {
  clear_screen();
  pid_t pid = fork();
  char portBuf[16], upBuf[16], downBuf[16], rightBuf[16], leftBuf[16], ipcBuf[16], wTypeBuf[16];
  char widthBuf[16], heightBuf[16], obstBuf[16], replBuf[16], kBuf[16], serModeBuf[16];

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
      NULL     //end for exec
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


/*
 * Creates a server by loading all the necessary data from loadPath with few from user.
 */
void load_server(int serverLoadType,
                 int type, int port,
                 int replications, char* loadPath, char* savePath) {
  clear_screen();

  pid_t pid = fork();
  char serModeBuf[16], ipcBuf[16], portBuf[16], replBuf[16];

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
    char *args[] = {
      "server",
      serModeBuf,
      "sock",
      portBuf,
      loadPath,
      replBuf,
      savePath,
      NULL     //end of exec()
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
