#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "ipcPipe.h"

pipe_t pipe_init(const char* name) {
  pipe_t p;
  mkfifo(name, 0666);
  p.fdWrite = open(name, O_WRONLY);
  p.fdRead  = open(name, O_RDONLY);
  return p;
}

void pipe_send(pipe_t* p, const void* buf, size_t len) {
  write(p->fdWrite, buf, len);
}

void pipe_recv(pipe_t* p, void* buf, size_t len) {
  read(p->fdRead, buf, len);
}

void pipe_close(pipe_t* p) {
  close(p->fdRead);
  close(p->fdWrite); 
}

