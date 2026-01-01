#include "ipcSocket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>

socket_t socket_init_server(int port) {
  socket_t s;
  int srv = socket(AF_INET, SOCK_STREAM, 0);
  if (srv < 0) exit(1);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("IPC socket server: bind failed\n");
    s.fd = -1;
    return s;
  };
  printf("IPC socket server: trying listen\n");
  listen(srv, 5);

  s.fd = srv;
  return s;
}

socket_t server_accept_client(int sfd) {
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sfd, &readfds);

  struct timeval tv;
  tv.tv_sec = WAIT_SECONDS;
  tv.tv_usec = 0;

  // čaká max. timeout_ms na dáta
  int result = select(sfd + 1, &readfds, NULL, NULL, &tv);
  socket_t s;

  if (result == 0) {
    s.fd = 0;
    return s;
  }
  if (result < 0) {
    s.fd = -1;
    return s;
  }

  int cfd = accept(sfd, NULL, NULL);
  s.fd = cfd;
  return s;
}

socket_t socket_init_client(const char* addrStr, int port) {
  socket_t s;
  s.fd = socket(AF_INET, SOCK_STREAM, 0);
  if (s.fd < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, addrStr, &addr.sin_addr) <= 0) {
    perror("inet_pton");
    exit(2);
  }

  if (connect(s.fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("IPC socket client: Connection failed");
    exit(3);
  }

  return s;
}

int socket_send(socket_t* s, const void* buf, size_t len) {
  return send(s->fd, buf, len, 0);
}

int socket_recv(socket_t* s, void* buf, size_t len) {
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(s->fd, &readfds);

  struct timeval tv;
  tv.tv_sec = WAIT_SECONDS;
  tv.tv_usec = 0;

  // čaká max. timeout_ms na dáta
  int result = select(s->fd + 1, &readfds, NULL, NULL, &tv);

  if (result == 0) {
    return -1;
  }
  if (result < 0) {
    return -1;
  }
  return recv(s->fd, buf, len, 0);
}

void socket_close(socket_t* s) { 
  close(s->fd);
}

