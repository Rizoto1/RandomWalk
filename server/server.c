#include "serverThreads.h"
#include <stdio.h>

//TODO
//change server so that it sends only data for set view mode e.g. if mode is interactive sends trajectory data if mode is summary, sends summary data based on client selected summary mode
//implement multiple users on server
//if everything works perfectly fine with socket implement other ipc protocols

//argc size 1(file path) + 2(ipc) + 4 (walker) + 4 (world) + 1(viewmode) + 3(simulation)  
int main(int argc, char** argv) {
  if (argc < 15) {
    perror("Server: invalid ammount of parameters\n");
    return 1;
  }

  return server_init(argv);
}

