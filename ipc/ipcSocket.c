#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct {
    int fd;
} socket_t;

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

    printf("IPC socket server: trying accept\n");
    s.fd = accept(srv, NULL, NULL);

    printf("IPC socket server: closing\n");
    close(srv);
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
  memset(&addr, 0, sizeof(addr));   // ← MUST FIX
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

void socket_send(socket_t* s, const void* buf, size_t len) {
  send(s->fd, buf, len, 0);
}

int socket_recv(socket_t* s, void* buf, size_t len) {
  return recv(s->fd, buf, len, 0);
}

void socket_close(socket_t* s) { 
  close(s->fd);
}

