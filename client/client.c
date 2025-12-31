#include "clientUtil.h"
#include "game/utility.h"
#include "game/world.h"
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

#define PORT 6666

/**
 * Receive thread - continuously receives packets from server
 * Packet:
 * Header + (w*h*2 doubles)
 */
void* thread_receive(void* arg) {
  client_context_t* ctx = (client_context_t*)arg;

  while (atomic_load(&ctx->running)) {
    clear_screen();
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
void* thread_send(void* arg) {
  client_context_t* ctx = (client_context_t*)arg;

  while (atomic_load(&ctx->running)) {
    char c = getchar();
    if (c == '\n') continue;

    socket_send(ctx->socket, &c, sizeof(c));

    if (c == 'q') {         // user requests quit
      atomic_store(&ctx->running, 0);
    }
  }
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
  memset(&ctx, 0, sizeof(ctx));
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
}

void newGame() {
  _Bool validInput = 0;

  int width, height, obstaclePercentage = 0;
  world_type_t worldType;
  printf("Enter world width and height:\n");
  scanf("%d", &width);
  scanf("%d", &height);
  if (width % 2 == 0) {
    width++;
  }
  if (height % 2 ==0) {
    height++;
  }
  printf("Enter world type:\n");
  printf("0) Without obstacles\n");
  printf("1) With obstacles\n");
  int i_worldType = 1;
  scanf("%d", &i_worldType);
  worldType = (world_type_t)i_worldType;
  if (worldType < 0) {
    worldType = 0;
  } else if (worldType > 1) {
    worldType = 1;
  }
  
  if (worldType == 1) {
  printf("Enter obstacle percentage as whole number:\n");
    scanf("%d", &obstaclePercentage);
    if (obstaclePercentage < 0) {
      obstaclePercentage = 0;
    } else if (obstaclePercentage > 100) {
      obstaclePercentage = 100;
    }
  }

  int replications;
  printf("Enter number of replications:\n");
  scanf("%d", &replications);

  double up, down, right, left;
  while (!validInput) {
    printf("Enter directional probabilities in order up, down, right, left in decimal numbers: \n");
    scanf("%lf", &up);
    scanf("%lf", &down);
    scanf("%lf", &right);
    scanf("%lf", &left);
    if(!validate_probabilities(&(probability_dir_t){up, down, right, left})) {
      printf("Sum of probabilities is not equal to 1.\n");
    } else {
      validInput = 1;
    }
  }

  int k;
  printf("Enter how many steps walker can make in single simulation:\n");
  scanf("%d", &k);

  char fPath[64];
  printf("Enter file name where simulation will be saved:\n");
  scanf("%63s", fPath);

  int ipc = 2;
  printf("Enter what type of IPC should server use:\n");
  printf("0) Pipe\n");
  printf("1) Semaphore\n");
  printf("2) Socket (default)\n");
  scanf("%d", &ipc);
  if (ipc < 0) {
    ipc = 0;
  } else if (ipc > 2) {
    ipc = 2;
  }

  viewmode_type_t viewMode;
  printf("Enter simulation view mode:\n");
  printf("0) Interactive - see walker trajectory\n");
  printf("1) Summary - statistical\n");
  int i_viewMode;
  scanf("%d", &i_viewMode);
  viewMode = (viewmode_type_t)i_viewMode;
  if (viewMode < 0) {
    viewMode = 0;
  } else if (viewMode > 1) {
    viewMode = 1;  
  }

  int num;
  printf("Would you like to create a simulation or return to main menu?\n");
  printf("1) Continue\n");
  printf("0) Exit\n");
  scanf("%d", &num);
  //if (num == 0) return;

  createServer(ipc,
               up, down, right, left,
               width, height, worldType, obstaclePercentage,
               viewMode,
               replications, k, fPath); 
}

void connectToGame() {
  socket_t sock;
  printf("Client: Initializing socket\n");
  sock = socket_init_client("127.0.0.1", PORT);
  if (sock.fd < 0) {
    perror("Failed to join");
    return;
  }

  printf("It works maybe?");

  client_context_t ctx = {0};
  ctx.socket = malloc(sizeof(socket_t));
  ctx.type = 2;
  ctx.socket->fd = sock.fd;
  simulation_menu(&ctx);
  free(ctx.socket);
}

void continueInGame() {

}

static void printMM() {
  clear_screen();
  printf("===== RANDOM WALK SIMULATION CLIENT =====\n");
  printf("1) New simulation\n");
  printf("2) Connect to simulation\n");
  printf("3) Rerun simulation\n");
  printf("0) Exit\n");
  printf("Choose: ");
}

void mainMenu() {
  int input;
  while (1) {
    printMM();
    scanf("%d", &input);
    if (input == 0) {
      return;
    }

    if (input == 1) {
      newGame();
    } else if (input ==2) {
      connectToGame();
    } else if ( input == 3) {
      continueInGame();
    } else {
      printf("Invalid input\n");
    }
  }

  return;
}


/**
 * MAIN - initializes client and launches send + receive threads
 */
int main(int argc, char** argv) {
  mainMenu();
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

