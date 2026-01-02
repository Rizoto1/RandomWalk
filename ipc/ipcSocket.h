#ifndef IPC_SOCKET_H
#define IPC_SOCKET_H

#include <unistd.h>

#define WAIT_SECONDS 5

typedef enum {
  PKT_SUMMARY,
  PKT_INTERACTIVE_MAP,
  PKT_INTERACTIVE_TRAJECTORY
} packet_type_t;

typedef struct {
  packet_type_t type;
  int cur, total, w, h, k;
} packet_header_t;

typedef struct {
  int fd;
} socket_t;

socket_t socket_init_server(int port);
socket_t socket_init_client(const char* ip, int port);
int socket_send(socket_t* s, const void* buf, size_t len);
int socket_recv(socket_t* s, void* buf, size_t size);
void socket_close(socket_t* s);
void socket_shutdown(socket_t* s);
socket_t server_accept_client(int sfd);

#endif

