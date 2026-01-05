#include "serverThreads.h"
#include <stdio.h>
#include <stdlib.h>

//argc size 1(file path) + 1 (load/create sim)
//load sim (0) - 2(ipc) load file path, replications, save file path
//create sim (1) - 2(ipc) + 4 (walker) + 4 (world) + 3(simulation)  
int main(int argc, char** argv) {
  if (argc < 2) {
    perror("Server: Invalid ammount of parameters\n");
    return 1;
  }

  if (atoi(argv[1]) == 0) {
    if (argc < 7) {
      perror("Server: Invalid ammount of parameters\n");
      return 1;
    }
  } else if (atoi(argv[1]) == 1) {
    if (argc < 15) {
      perror("Server: invalid ammount of parameters\n");
      return 1;
    }
  } else {
    perror("Server: Invalid server configuration type.\n");
    return 1;
  }

  return server_init(argv);
}
