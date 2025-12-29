#include "ipcPipe.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

pipe_t pipe_init_server(const char* name) {
    char in[64], out[64];
    sprintf(in, "%s_c2s", name);
    sprintf(out, "%s_s2c", name);
    mkfifo(in,0666);
    mkfifo(out,0666);
    pipe_t p;
    p.fd_read = open(in,O_RDONLY);
    p.fd_write = open(out,O_WRONLY);
    return p;
}

pipe_t pipe_init_client(const char* name) {
    char in[64], out[64];
    sprintf(in, "%s_s2c", name);
    sprintf(out, "%s_c2s", name);
    pipe_t p;
    p.fd_read = open(in,O_RDONLY);
    p.fd_write = open(out,O_WRONLY);
    return p;
}

void pipe_send(pipe_t* p, const char* msg) {
    write(p->fd_write, msg, strlen(msg));
}

void pipe_recv(pipe_t* p, char* buff, int size) {
    int n = read(p->fd_read, buff, size);
    buff[n] = 0;
}

void pipe_close(pipe_t* p) {
    close(p->fd_read);
    close(p->fd_write);
}

