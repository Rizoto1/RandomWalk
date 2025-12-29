#include "ipcSocket.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

socket_t socket_init_server(int port) {
    socket_t s;
    s.server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = INADDR_ANY;
    bind(s.server_fd,(void*)&a,sizeof(a));
    listen(s.server_fd,5);
    s.conn_fd = accept(s.server_fd,NULL,NULL);
    return s;
}

socket_t socket_init_client(const char* ip, int port) {
    socket_t s;
    struct sockaddr_in a = {0};
    s.server_fd = socket(AF_INET, SOCK_STREAM, 0);
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = inet_addr(ip);
    connect(s.server_fd,(void*)&a,sizeof(a));
    s.conn_fd = s.server_fd;
    return s;
}

void socket_send(socket_t* s, const char* msg) {
    send(s->conn_fd, msg, strlen(msg), 0);
}

void socket_recv(socket_t* s, char* buff, int size) {
    int r = recv(s->conn_fd, buff, size, 0);
    buff[r] = 0;
}

void socket_close(socket_t* s) {
    close(s->conn_fd);
    close(s->server_fd);
}

