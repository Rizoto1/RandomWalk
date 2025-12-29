#ifndef IPC_PIPE_H
#define IPC_PIPE_H

typedef struct {
  int fd_read;
  int fd_write;
} pipe_t;

pipe_t pipe_init_server(const char* name);
pipe_t pipe_init_client(const char* name);
void pipe_send(pipe_t* p, const char* msg);
void pipe_recv(pipe_t* p, char* buff, int size);
void pipe_close(pipe_t* p);

#endif

