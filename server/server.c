#include "serverThreads.h"
#include <stdio.h>

//argc size 1(file path) + 2(ipc) + 4 (walker) + 4 (world) + 1(viewmode) + 3(simulation)  
int main(int argc, char** argv) {
  if (argc < 15) {
    perror("Server: invalid ammount of parameters\n");
    return 1;
  }

  return server_init(argv);
}

