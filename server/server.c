#include "serverThreads.h"
#include <stdio.h>
#include <stdlib.h>

//TODO
//implement if server is for one or more clients
//  this is sent by client

//argc size 1(file path) + 1 (load/create sim)
//load sim (0) - 2(ipc) load file path, replications, save file path, clients(0 - one client, 1 - SERVER_CAPACITY)
//create sim (1) - 2(ipc) + 4 (walker) + 4 (world) + 3(simulation) + clients(0 - one client, 1 - SERVER_CAPACITY) 
int main(int argc, char** argv) {
  if (argc < 2) {
    perror("Server: Invalid ammount of parameters\n");
    return 1;
  }

  if (atoi(argv[1]) == 0) {
    if (argc < 8) {
      perror("Server: Invalid ammount of parameters\n");
      return 1;
    }
  } else if (atoi(argv[1]) == 1) {
    if (argc < 16) {
      perror("Server: invalid ammount of parameters\n");
      return 1;
    }
  } else {
    perror("Server: Invalid server configuration type.\n");
    return 1;
  }

  return server_init(argv);
}
