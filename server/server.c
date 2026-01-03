#include "serverThreads.h"
#include <stdio.h>

//TODO
//if everything works perfectly fine with socket implement other ipc protocols
//summary to interactive create memory leaks and if client quits some major issues happen with thread joining

//argc size 1(file path) + 2(ipc) + 4 (walker) + 4 (world) + 1(viewmode) + 3(simulation)  
int main(int argc, char** argv) {
  if (argc < 15) {
    perror("Server: invalid ammount of parameters\n");
    return 1;
  }

  return server_init(argv);
}
