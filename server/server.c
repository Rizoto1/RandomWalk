#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <ipc/ipcSocket.h>
#include "serverUtil.h"

void* server_recv_thread(void* arg) {
    ServerCtx* ctx = arg;
    char buf[64];

    while(*ctx->running) {
        if(ctx->type==0) pipe_recv(ctx->pipe,buf,64);
        if(ctx->type==1) shm_read(ctx->shm,buf,64);
        if(ctx->type==2) socket_recv(ctx->sock,buf,64);

        if(strcmp(buf,"STOP")==0) *ctx->running = false;
        if(strcmp(buf,"MODE_INTER")==0) ctx->sim->interactive = 1;
        if(strcmp(buf,"MODE_SUM")==0)   ctx->sim->interactive = 0;
    }
    return NULL;
}

void* server_send_thread(void* arg) {
    ServerCtx* ctx = arg;
    char message[256];

    while(*ctx->running) {
        sprintf(message,"%d/%d",ctx->sim->currentReplication,ctx->sim->replications);

        if(ctx->type==0) pipe_send(ctx->pipe,message);
        if(ctx->type==2) socket_send(ctx->sock,message);
        if(ctx->type==1) shm_write(ctx->shm,&ctx->sim->currentReplication); // 🔁 SHM nám posiela len číslo

        sleep(1);
    }
    return NULL;
}

void* simulation_thread(void* arg) {
    ServerCtx* ctx = arg;

    while(*ctx->running && ctx->sim->currentReplication < ctx->sim->replications) {
        simulation_step(ctx->sim);
        ctx->sim->currentReplication++;
    }

    simulation_save(ctx->sim);
    *ctx->running = false;
    return NULL;
}

int main(int argc, char** argv) {

    atomic_bool running = true;
    simulation_t* sim = simulation_create(params);   // ty dodáš implementáciu

    ServerCtx ctx = {0};
    ctx.running = &running;
    ctx.sim = sim;

    if(strcmp(argv[1],"pipe")==0) {
        Pipe p = pipe_init_server(argv[2]);
        ctx.pipe = malloc(sizeof(Pipe)); *ctx.pipe = p; ctx.type = 0;
    }
    if(strcmp(argv[1],"sock")==0) {
        Socket s = socket_init_server(atoi(argv[2]));
        ctx.sock = malloc(sizeof(Socket)); *ctx.sock = s; ctx.type = 2;
    }
    if(strcmp(argv[1],"shm")==0) {
        Shm s = shm_init_server(atoi(argv[2]), sim);
        ctx.shm = malloc(sizeof(Shm)); *ctx.shm = s; ctx.type = 1;
    }

    pthread_t tr, ts, tsim;
    pthread_create(&tr,NULL,server_recv_thread,&ctx);
    pthread_create(&ts,NULL,server_send_thread,&ctx);
    pthread_create(&tsim,NULL,simulation_thread,&ctx);

    pthread_join(tr,NULL);
    pthread_join(ts,NULL);
    pthread_join(tsim,NULL);
}

