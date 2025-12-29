#ifndef IPC_SOCKET_H
#define IPC_SOCKET_H

typedef struct {
  int server_fd;
  int conn_fd;
} socket_t;

socket_t socket_init_server(int port);
socket_t socket_init_client(const char* ip, int port);
void socket_send(socket_t* s, const char* msg);
void socket_recv(socket_t* s, char* buff, int size);
void socket_close(socket_t* s);

#endif

