#include "serverThreads.h"
#include <stdio.h>

//TODO
//if everything works perfectly fine with socket implement other ipc protocols
//when changing from summary a to b it stops working, summary to interactive works
//send or recv thread keeps getting stuck in loop?


//argc size 1(file path) + 2(ipc) + 4 (walker) + 4 (world) + 1(viewmode) + 3(simulation)  
int main(int argc, char** argv) {
  if (argc < 15) {
    perror("Server: invalid ammount of parameters\n");
    return 1;
  }

  return server_init(argv);
}

