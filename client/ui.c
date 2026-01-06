#include "ui.h"
#include "clientUtil.h"
#include "clientThreads.h"
#include "game/world.h"
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

/*
 * Draws summary view mode.
 */
void draw_summary_map(double* buf, packet_header_t* hdr) {
  printf("\n   Replication %d / %d\n", hdr->cur, hdr->total);
  printf("   View mode: Summary\n");

  for (int i = 0; i < hdr->h; i++) {
    printf("   ");
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

  printf("\n   Replication %d / %d\n", hdr->cur, hdr->total);
  printf("   View mode: Interactive\n");
  printf("   Steps %d / %d\n", count, hdr->k - 1);
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

/*
 * Creates server based on user inputs.
 */
void new_game(void) {
  //width
  _Bool invalidInput = 0;
  int width;
  while (1) {
    if (invalidInput) {
      print_article(0);
      print_message("Invalid width.\n", 1, 0);
    } else {
      print_article(1);
    }

    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    print_message("Please insert world width.\nWidth: ", 0, 1);

    int rs = read_input(INPUT_INT, &width, 0);
    if (rs == 1) return;
    else if (rs == 0 &&
            width > 0) break;

    invalidInput = 1;
  }

  //height
  invalidInput = 0;
  int height;
  while (1) {
    if (invalidInput) {
      print_article(0);
      print_message("Invalid height.\n", 1, 0);
    } else {
      print_article(1);
    }

    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    print_message("Please insert world height.\nHeight: ", 0, 1);

    int rs = read_input(INPUT_INT, &height, 0);
    if (rs == 1) return;
    else if (rs == 0 &&
            height > 0) break;

    invalidInput = 1;
  }

  if (width % 2 == 0) {
    width++;
  }
  if (height % 2 == 0) {
    height++;
  }

  //world type
  invalidInput = 0;
  world_type_t worldType;
  while (1) {
    if (invalidInput) {
      print_article(0);
      print_message("Invalid world type.\n", 1, 0);
    } else {
      print_article(1);
    }
    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    print_message("Please insert world type.\n0) Without obstacles\n1) With obstacles\nType: ", 0, 1);
    int rs = read_input(INPUT_INT, &worldType, 0);
    if (rs == 1) return;
    else if (rs == 0 &&
            worldType >= 0 && worldType <=1) break;

    invalidInput = 1;
  }

  //obstaclePercentage if world is with obstacles
  invalidInput = 0;
  int obstaclePercentage = 0;
  if (worldType == W_OBSTALCES) {
    while (1) {
      if (invalidInput) {
        print_article(0);
        print_message("Invalid obstacle percentage.\n", 1, 0);
      } else {
        print_article(1);
      }

      print_message("If you want to exit anytime type 'q'\n", 0, 1);
      print_message("Please insert obstacle percentage as a whole number.\nPercentage: ", 0, 1);

      int rs = read_input(INPUT_INT, &obstaclePercentage, 0);
      if (rs == 1) return;
      else if (rs == 0 &&
              obstaclePercentage >= 0 && obstaclePercentage <= 100) break;
        invalidInput = 1;
      }
  }

  //replications
  int replications;
  invalidInput = 0;
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
    else if (rs == 0 && replications > 0) break;
    invalidInput = 1;
  } 

  //probabilites
  int up, down, right, left = 25;
  double f_up, f_down, f_right, f_left = 0;
  while (1) {

    //up
    invalidInput = 0;
    while (1) {
      if (invalidInput) {
        print_article(0);
        print_message("Invalid probability.\n", 1, 0);
      } else {
        print_article(1);
      }
      print_message("If you want to exit anytime type 'q'\n", 0, 1);
      print_message("Please enter probability for walker to move up. Enter as a whole number between 0 - 100.\nProbability: ", 0, 1);
      int rs = read_input(INPUT_INT, &up, 0);
      if (rs == 1) return;
      else if (rs == 0 && up >= 0 && up <= 100) {
        f_up = (double)up / 100;
        break;
      }
      invalidInput = 1;
    } 

    //down
    invalidInput = 0;
    while (1) {
      if (invalidInput) {
        print_article(0);
        print_message("Invalid probability.\n", 1, 0);
      } else {
        print_article(1);
      }
      print_message("If you want to exit anytime type 'q'\n", 0, 1);
      print_message("Please enter probability for walker to move down. Enter as a whole number between 0 - 100.\nProbability: ", 0, 1);
      int rs = read_input(INPUT_INT, &down, 0);
      if (rs == 1) return;
      else if (rs == 0 && down >= 0 && down <= 100) { 
        f_down = (double)down / 100;
        break;
      }
      invalidInput = 1;
    } 

    //right
    invalidInput = 0;
    while (1) {
      if (invalidInput) {
        print_article(0);
        print_message("Invalid probability.\n", 1, 0);
      } else {
        print_article(1);
      }
      print_message("If you want to exit anytime type 'q'\n", 0, 1);
      print_message("Please enter probability for walker to move right. Enter as a whole number between 0 - 100.\nProbability: ", 0, 1);
      int rs = read_input(INPUT_INT, &right, 0);
      if (rs == 1) return;
      else if (rs == 0 && right >= 0 && right <= 100) { 
        f_right = (double)right / 100;
        break;
      }
      invalidInput = 1;
    }

    //left
    invalidInput = 0;
    while (1) {
      if (invalidInput) {
        print_article(0);
        print_message("Invalid probability.\n", 1, 0);
      } else {
        print_article(1);
      }
      print_message("If you want to exit anytime type 'q'\n", 0, 1);
      print_message("Please enter probability for walker to move left. Enter as a whole number between 0 - 100.\nProbability: ", 0, 1);
      int rs = read_input(INPUT_INT, &left, 0);
      if (rs == 1) return;
      else if (rs == 0 && left >= 0 && left <= 100) { 
        f_left = (double)left / 100;
        break;
      }
      invalidInput = 1;
    }

    //validation
    if (validate_probabilities(&(probability_dir_t){f_up, f_down, f_right, f_left})) {
      break;
    }
    print_article(0);
    print_message("Probabilities don't add up to 100,\n", 1, 0);
    sleep(2);
  }

  //k
  invalidInput = 0;
  int k;
  while (1) {
    if (invalidInput) {
      print_article(0);
      print_message("Invalid steps.\n", 1, 0);
    } else {
      print_article(1);
    }

    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    print_message("Please enter maximum ammount of steps walker can make.\nSteps: ", 0, 1);

    int rs = read_input(INPUT_INT, &k, 0);
    if (rs == 1) return;
    else if (rs == 0 &&
            k > 0) break;

    invalidInput = 1;
  }

  //save file
  print_article(1);
  print_message("If you want to exit anytime type 'q'\n", 0, 1);
  char fSavePath[64];
  print_message("Please enter file name to which simulation will be saved.\nFile name: ", 0, 1);
  if (read_input(INPUT_STRING, fSavePath, 64)) return;

  //ipc 
  print_article(1);
  int ipc = 2;
  print_message("Please enter what type of IPC should server use:\n0) Pipe - not implemented yet\n1) Semaphore - not implemented yet\n2) Socket (default)\nIPC: --------\n", 0, 1);
  sleep(1);

  //port
  int port = 0;
  invalidInput = 0;
  while (1) {
    if (invalidInput) {
      print_article(0);
      print_message("Invalid port.\n", 1, 0);
    } else {
      print_article(1);
    }
    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    print_message("Please insert port the port should be from 0 to 9999.\nPort: ", 0, 1);
    int rs = read_input(INPUT_INT, &port, 0);
    if (rs == 1) return;
    else if (rs == 0 &&
            port >= 0 && port < 10000) break;

    invalidInput = 1;
  }

  create_server(ipc, port,
                f_up, f_down, f_right, f_left,
                width, height, worldType, obstaclePercentage,
                1,
                replications, k, fSavePath); 
}

/*
 *  Connects client to a server based on port the user typed in.
 */
void connect_to_game(void) {
  //port
  _Bool invalidInput = 0;
  int port;
  while (1) {
    if (invalidInput) {
      print_article(0);
      print_message("Invalid port.\n", 1, 0);
    } else {
      print_article(1);
    }
    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    print_message("Please insert port the port should be from 0 to 9999.\nPort: ", 0, 1);
    int rs = read_input(INPUT_INT, &port, 0);
    if (rs == 1) return;
    else if (rs == 0 &&
        port >= 0 && port < 10000) {
    break;
  }
    invalidInput = 1;
  }

  //ipc 
  print_article(1);
  print_message("Please enter what type of IPC should server use:\n0) Pipe - not implemented yet\n1) Semaphore - not implemented yet\n2) Socket (default)\nIPC: --------\n", 0, 1);
  sleep(1);

  //init ipc
  ipc_ctx_t ipc;
  if(ipc_init(&ipc, 1, "sock", port)) {
    perror("Client: Connection init failed\n");
    return;
  };

  //init client
  client_context_t ctx;
  if(ctx_init(&ctx, &ipc)) {
    perror("Client: Client init failed\n");
    return;
  };

  //start sim. menu
  simulation_menu(&ctx);

  //destroys
  atomic_store(&ctx.running, 0);
  socket_shutdown(&ctx.ipc->sock);
  ipc_destroy(ctx.ipc);
  ctx_destroy(&ctx);
}

/*
 * Gets necessary data from user to load server from a file.
 */
void load_game(void) {
  _Bool invalidInput = 0;

  //replications
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
    else if (rs == 0 && replications > 0) break;
    invalidInput = 1;
  } 

  //port
  invalidInput = 0;
  int port;
  while (1) {
    if (invalidInput) {
      print_article(0);
      print_message("Invalid port.\n", 1, 0);
    } else {
      print_article(1);
    }
    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    print_message("Please insert port the port should be from 0 to 9999.\nPort: ", 0, 1);
    int rs = read_input(INPUT_INT, &port, 0);
    if (rs == 1) return;
    else if (rs == 0 &&
        port >= 0 && port < 10000) {
    break;
  }
    invalidInput = 1;
  }

  //load file
  invalidInput = 0;
  char fLoadPath[64];
  while (1) {
    if (invalidInput) {
      print_article(0);
      print_message("File does not exist.\n", 1, 0);
    } else {
      print_article(1);
    }
    print_message("If you want to exit anytime type 'q'\n", 0, 1);
    print_message("Please enter file name from which simulation will be loaded.\nFile name: ", 0, 1);
    int result = read_input(INPUT_STRING, fLoadPath, 64);
    if (result ==1) return;
    FILE* f = fopen(fLoadPath, "r");
    if(!f) {
      invalidInput = 1;
      continue;
    }  
    fclose(f);
    break;
  }

  //save file
  print_article(1);
  print_message("If you want to exit anytime type 'q'\n", 0, 1);
  char fSavePath[64];
  print_message("Please enter file name to which simulation will be saved.\nFile name: ", 0, 1);
  if (read_input(INPUT_STRING, fSavePath, 64)) return;

  //ipc 
  print_article(1);
  int ipc = 2;
  print_message("Please enter what type of IPC should server use:\n0) Pipe - not implemented yet\n1) Semaphore - not implemented yet\n2) Socket (default)\nIPC: --------\n", 0, 1);
  sleep(1);

  //load server from file
  load_server(0, ipc, port, replications, fLoadPath, fSavePath);
  sleep(3);
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

