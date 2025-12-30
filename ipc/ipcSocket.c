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

    bind(srv, (struct sockaddr*)&addr, sizeof(addr));
    listen(srv, 1);

    s.fd = accept(srv, NULL, NULL);
    close(srv);
    return s;
}

socket_t socket_init_client(const char* addrStr, int port) {
    socket_t s;
    s.fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, addrStr, &addr.sin_addr);

    if (connect(s.fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) exit(2);
    return s;
}

void socket_send(socket_t* s, const void* buf, size_t len) {
    send(s->fd, buf, len, 0);
}

void socket_recv(socket_t* s, void* buf, size_t len) {
    recv(s->fd, buf, len, 0);
}

void socket_close(socket_t* s) { close(s->fd); }

