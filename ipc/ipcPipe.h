#ifndef IPC_PIPE_H
#define IPC_PIPE_H

#include <unistd.h>

typedef struct {
  int fdRead;
  int fdWrite;
} pipe_t;

pipe_t pipe_init_server(const char* name);
pipe_t pipe_init_client(const char* name);
void pipe_send(pipe_t* p, const void* msg, size_t len);
void pipe_recv(pipe_t* p, void* buff, size_t len);
void pipe_close(pipe_t* p);

#endif

