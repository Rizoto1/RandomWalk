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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * Draws summary view mode.
 */
void draw_summary_map(double* buf, packet_header_t* hdr) {
  printf("\nReplication %d / %d\n", hdr->cur, hdr->total);

  for (int i = 0; i < hdr->h; i++) {
    for (int j = 0; j < hdr->w; j++) {
      int idx = (i * hdr->w + j);
      if (buf[idx] == -1) {
        printf("  #  ");
        continue;
      }
      printf("%.2f ", buf[idx]);
    }
    printf("\n");
  }

}

/*
 * Draws interactive view mode.
 */
void draw_interactive_map(const char* world, position_t* path, packet_header_t* hdr) {
  int w = hdr->w;
  int h = hdr->h;
  int count = hdr->count;

  printf("\nReplication %d / %d\n", hdr->cur, hdr->total);
  position_t center = {(int)floor((double)hdr->w / 2), (int)floor((double)hdr->h / 2)};

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;

      //walker
      if (path[count-1].x == x && path[count-1].y == y) {
        printf(" C ");     // C = current position
        continue;
      }

      //walker starting pos
      if (path[0].x == x && path[0].y == y) {
        printf(" x ");
        continue;
      }

      //walker trajectory
      _Bool printed = 0;
      for (int p = 0; p < count-1; p++) {
        if (path[p].x == x && path[p].y == y) {
          printf(" * ");
          printed = 1;
          break;
        }
      }
      if (printed) continue;

      //obstacle
      if (world[idx] == 1) {
        printf(" # ");
        continue;
      }

      //middle
      if (x == center.x && y == center.y) {
        printf(" + ");
        continue;
      }

      //empty point
      printf(" . ");
    }
    printf("\n");
  }
}

void new_game(void) {
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

  create_server(ipc, port,
               up, down, right, left,
               width, height, worldType, obstaclePercentage,
               1,
               replications, k, fPath); 
}

void connect_to_game(void) {
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

/*
 * Gets necessary data from user to run server.
 */
void load_game(void) {
  _Bool invalidInput = 0;

  int replications;
  while (1) {
    if (invalidInput) {
      print_article(0);
      print_message("Invalid input.\n", 1, 0);
    } else {
      print_article(1);
    }
    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    print_message("Please enter number of replications the simulation should replicate.\nReplications: ", 0, 1);
    int rs = read_input(INPUT_INT, &replications, 0);
    if (rs == 1) return;
    else if (rs == 0) break;
    invalidInput = 1;
  } 

  int port;
  while (1) {
    if (invalidInput) {
      print_article(0);
      print_message("Invalid port.\n", 1, 0);
    } else {
      print_article(1);
    }
    print_article(1);
    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    print_message("Please insert port the port should be from 0 to 9999.\nPort: ", 0, 1);
    int rs = read_input(INPUT_INT, &port, 0);
    if (rs == 1) return;
    else if (rs == 0 &&
        port >= 0 && port < 10000) break;
    invalidInput = 1;
  }
  invalidInput = 0;

  while (1) {
    if (invalidInput) {
      print_article(0);
      print_message("File does not exist.\n", 1, 0);
    } else {
      print_article(1);
    }
    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    char fLoadPath[64];
    print_message("Please enter file name from which simulation will be loaded.\nFile name: ", 0, 1);
    int result = read_input(INPUT_STRING, fLoadPath, 64);
    if (result ==1) return;
    else if (result == 0) {
      FILE* f = fopen(fLoadPath, "r");
      if (!f) invalidInput = 1;
      else {
        fclose(f);
        break;
      }
    }

    print_article(1);
    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    char fSavePath[64];
    print_message("Please enter file name to which simulation will be saved.\nFile name: ", 0, 1);
    if (read_input(INPUT_STRING, fSavePath, 64)) return;

    print_article(1);
    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    int ipc = 2;
    print_message("Please enter what type of IPC should server use:\n0) Pipe - not implemented yet\n1) Semaphore - not implemented yet\n2) Socket (default)\nIPC: --------\n", 0, 1);
    sleep(1);

    load_server(0, ipc, port, replications, fLoadPath, fSavePath);
  }
}

  /*
 * This is main loop in which user is during clients lifetime.
 * Based on user input it branches:
 * 1 - create new game
 * 2 - connect to game
 * 3 - load game from file
 * 0 - exit
 */
  void main_menu(void) {
    char input;
    print_article(1);
    print_mm();
    while (1) {
      scanf("%c", &input);
      if (input == '0') {
        break;
      }

      if (input == '1') {
        new_game();
        print_article(1);
        print_mm();
      } else if (input == '2') {
        connect_to_game();
        print_article(1);
        print_mm();
      } else if ( input == '3') {
        load_game();
        print_article(1);
        print_mm();
      } else {
        clear_screen();
        print_article(0);
        print_message("Invalid input.\n", 1, 0);
        print_mm();
      }
    }
    clear_screen();
    return;
  }

