#include "ipcSocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>

/*
 * Initializes socket for server based on port.
 * If it succeeds return socket_t with fd > 0, otherwise returns socket_t with fd == -1.
 */
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

/*
 * Accepts clients and listens for them in a time interval.
 * If it accepts new client return socket_t with fd > 0, otherwise returns socket_t with fd <= 0.
 */
socket_t server_accept_client(int sfd) {
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sfd, &readfds);

  struct timeval tv;
  tv.tv_sec = WAIT_SECONDS;
  tv.tv_usec = 0;

  int result = select(sfd + 1, &readfds, NULL, NULL, &tv);
  socket_t s = {-1};

  if (result <= 0) {
    return s;
  }

  if (!FD_ISSET(sfd, &readfds)) {
    return s;
  }

  int cfd = accept(sfd, NULL, NULL);
  if (cfd < 0) {
    perror("Server: Failed to accept.\n");
    return s;
  }

  s.fd = cfd;
  return s;
}

/*
 * Initializes socket for client.
 * If it succeds returns socket_t with fd > 0, otherwise fd is <= 0.
 */
socket_t socket_init_client(const char* addrStr, int port) {
  if (!addrStr) return (socket_t){-1};
  socket_t s;
  s.fd = socket(AF_INET, SOCK_STREAM, 0);
  if (s.fd < 0) {
    perror("socket");
    return (socket_t){-1};
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, addrStr, &addr.sin_addr) <= 0) {
    perror("inet_pton");
    return (socket_t){-1};
  }

  if (connect(s.fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("IPC socket client: Connection failed");
    return (socket_t){-1};
  }

  return s;
}

/*
 * Sends socket.
 */
int socket_send(socket_t* s, const void* buf, size_t len) {
  if(!s || !buf) return -1;
  return send(s->fd, buf, len, 0);
}

/*
 * Receives socket.
 */
int socket_recv(socket_t* s, void* buf, size_t len) {
  if(!s || !buf) return -1;
  /*fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(s->fd, &readfds);

  struct timeval tv;
  tv.tv_sec = WAIT_SECONDS;
  tv.tv_usec = 0;

  int result = select(s->fd + 1, &readfds, NULL, NULL, &tv);

  if (result == 0) {
    return -1;
  }
  if (result < 0) {
    return -1;
  }*/
  return recv(s->fd, buf, len, 0);
}

/*
 * Closes socket.
 */
void socket_close(socket_t* s) { 
  if(!s) return;
  close(s->fd);
}

/*
 * Shutsdown socket.
 */
void socket_shutdown(socket_t* s) {
  if(!s) return;
  shutdown(s->fd, SHUT_RDWR);
}
