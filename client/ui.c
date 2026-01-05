#include "ui.h"
#include "clientUtil.h"
#include "clientThreads.h"
#include "ipc/ipcUtil.h"
#include <ipc/ipcSocket.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <game/simulation.h>
#include <game/walker.h>
#include <math.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#define PADDING 

void draw_interactive_map(const char* world, position_t* path, packet_header_t* hdr) {
  int w = hdr->w;
  int h = hdr->h;
  int count = hdr->count;

  printf("\nReplication %d / %d\n", hdr->cur, hdr->total);
  position_t center = {(int)floor((double)hdr->w / 2), (int)floor((double)hdr->h / 2)};

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;

      // 1️⃣ ak je toto pozícia chodca (posledný krok)
      if (path[count-1].x == x && path[count-1].y == y) {
        printf(" C ");     // C = current position
        continue;
      }

      if (path[0].x == x && path[0].y == y) {
        printf(" x ");
        continue;
      }

      // 2️⃣ ak je toto súčasť trajektórie (hociaké predchádzajúce x,y)
      _Bool printed = 0;
      for (int p = 0; p < count-1; p++) {
        if (path[p].x == x && path[p].y == y) {
          printf(" * ");
          printed = 1;
          break;
        }
      }
      if (printed) continue;

      // 3️⃣ prekážka
      if (world[idx] == 1) {
        printf(" # ");
        continue;
      }

      // 4️⃣ stred
      if (x == center.x && y == center.y) {
        printf(" + ");
        continue;
      }

      // 5️⃣ prázdne políčko
      printf(" . ");
    }
    printf("\n");
  }
}


static void printMM(const char* msg) {
  clear_screen();
printf(
        "       ██████╗  █████╗ ███╗   ██╗██████╗  ██████╗ ███╗   ███╗\n"
        "       ██╔══██╗██╔══██╗████╗  ██║██╔══██╗██╔═══██╗████╗ ████║\n"
        "       ██████╔╝███████║██╔██╗ ██║██║  ██║██║   ██║██╔████╔██║\n"
        "       ██╔══██╗██╔══██║██║╚██╗██║██║  ██║██║   ██║██║╚██╔╝██║\n"
        "       ██║  ██║██║  ██║██║ ╚████║██████╔╝╚██████╔╝██║ ╚═╝ ██║\n"
        "       ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚═╝     ╚═╝\n"
        "\n"
        "               ██╗    ██╗ █████╗ ██╗     ██╗  ██╗\n"
        "               ██║    ██║██╔══██╗██║     ██║ ██╔╝\n"
        "               ██║ █╗ ██║███████║██║     █████╔╝ \n"
        "               ██║███╗██║██╔══██║██║     ██╔═██╗ \n"
        "               ╚███╔███╔╝██║  ██║███████╗██║  ██╗\n"
        "                ╚══╝╚══╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝\n"
        "\n"
        "            ---------------------------------\n"
        "\n"
        "                    [ 1 ]  NEW GAME\n"
        "                    [ 2 ]  CONNECT\n"
        "                    [ 3 ]  RERUN\n"
        "                    [ 0 ]  EXIT\n"
        "\n"
        "            ---------------------------------\n"
    );
  
  
  if (msg) {
    msg += '\0';
    printf(
        "                    Error: %s",
      msg);
  }
  printf(
        "                    Choose: ");
}

void newGame(void) {
  clear_screen();
  int width, height, obstaclePercentage = 0;
  world_type_t worldType;
  printf("Enter world width and height:\n");
  scanf("%d", &width);
  scanf("%d", &height);
  if (width % 2 == 0) {
    width++;
  }
  if (height % 2 == 0) {
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

  double up, down, right, left = 0.25;
  _Bool validInput = 0;
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
  /*scanf("%d", &ipc);
  if (ipc < 0) {
    ipc = 0;
  } else if (ipc > 2) {
    ipc = 2;
  }*/

  int port = 0;
  while (1) {
    printf("Please insert port the port should be from 0 to 9999. \n Insert: ");
    scanf("%d", &port); 
    if (port >= 0 && port < 10000) {
      break;
    }
    printf("Invalid port\n");
  }

  int num;
  printf("Would you like to create a simulation or return to main menu?\n");
  printf("1) Continue\n");
  printf("0) Exit\n");
  scanf("%d", &num);
  //if (num == 0) return;

  createServer(ipc, port,
               up, down, right, left,
               width, height, worldType, obstaclePercentage,
               1,
               replications, k, fPath); 
}

void connectToGame(void) {
  clear_screen();
  int port;
  while (1) {
    printf("Please insert port the port should be from 0 to 9999. \n Insert: ");
    scanf("%d", &port); 
    if (port >= 0 && port < 10000) {
      break;
    }
    printf("Invalid port\n");
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

  simulation_menu(&ctx);
  
  atomic_store(&ctx.running, 0);
  socket_shutdown(&ctx.ipc->sock);
  ipc_destroy(ctx.ipc);
  ctx_destroy(&ctx);
}

void continueInGame(void) {
  clear_screen();
  int replications;
  printf("Enter number of replications:\n");
  scanf("%d", &replications);
  
  int port;
  while (1) {
    printf("Please insert port the port should be from 0 to 9999. \n Insert: ");
    scanf("%d", &port); 
    if (port >= 0 && port < 10000) {
      break;
    }
    printf("Invalid port\n");
  }
  
  char fLoadPath[64];
  printf("Enter file name from which simulation will be loaded:\n");
  scanf("%63s", fLoadPath);

  char fSavePath[64];
  printf("Enter file name where simulation will be saved:\n");
  scanf("%63s", fSavePath);
  
  int ipc = 2;
  printf("Enter what type of IPC should server use:\n");
  printf("0) Pipe\n");
  printf("1) Semaphore\n");
  printf("2) Socket (default)\n");
  /*scanf("%d", &ipc);
  if (ipc < 0) {
    ipc = 0;
  } else if (ipc > 2) {
    ipc = 2;
  }*/

  loadServer(0, ipc, port, replications, fLoadPath, fSavePath);
}

void mainMenu(void) {
  char input;
  printMM(NULL);
  while (1) {
    scanf("%c", &input);
    if (input == '0') {
      return;
    }

    if (input == '1') {
      newGame();
    } else if (input == '2') {
      connectToGame();
    } else if ( input == '3') {
      continueInGame();
    } else {
      printMM("Invalid input. \n");
    }
  }
  clear_screen();
  return;
}

