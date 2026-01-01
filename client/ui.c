#include "ui.h"
#include "clientUtil.h"
#include "clientThreads.h"
#include <ipc/ipcSocket.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <game/simulation.h>
#include <game/walker.h>
#include <math.h>

void draw_interactive_map(char* world, position_t* path, packet_header_t* hdr) {
  clear_screen();
  int w = hdr->w;
  int h = hdr->h;
  int k = hdr->k;

  printf("\nReplication %d / %d\n", hdr->cur + 1, hdr->total);
  position_t center = {(int)floor((double)hdr->w / 2), (int)floor((double)hdr->h / 2)};

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;

      // 1️⃣ ak je toto pozícia chodca (posledný krok)
      if (path[k-1].x == x && path[k-1].y == y) {
        printf(" C ");     // C = current position
        continue;
      }

      // 2️⃣ ak je toto súčasť trajektórie (hociaké predchádzajúce x,y)
      _Bool printed = 0;
      for (int p = 0; p < k-1; p++) {
        if (path[p].x == x && path[p].y == y) {
          printf(" * ");
          printed = 1;
          break;
        }
      }
      if (printed) continue;

      // 3️⃣ prekážka
      if (world[idx] == '1') {
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


static void printMM() {
  clear_screen();
  printf("===== RANDOM WALK SIMULATION CLIENT =====\n");
  printf("1) New simulation\n");
  printf("2) Connect to simulation\n");
  printf("3) Rerun simulation\n");
  printf("0) Exit\n");
  printf("Choose: ");
}

void newGame() {
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

  client_context_t ctx;
  ctx_init(&ctx);
  ctx.socket = &sock;
  ctx.type = 2;
  ctx.socket->fd = sock.fd;
  simulation_menu(&ctx);
  close(sock.fd);
  ctx_destroy(&ctx);
}
void continueInGame() {

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

