#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <ipc/ipcSocket.h>
#include "game/simulation.h"
#include "serverUtil.h"
#include <unistd.h>

void* server_recv_thread(void* arg) {
  ServerCtx* ctx = arg;
  char buf[64];

  while(*ctx->running) {
    if(ctx->type==0) pipe_recv(ctx->pipe,buf,64);
    if(ctx->type==1) shm_read(ctx->shm,buf,64);
    if(ctx->type==2) socket_recv(ctx->sock,buf,64);

    if(strcmp(buf,"STOP")==0) *ctx->running = 0;
    if(strcmp(buf,"MODE_INTER")==0) ctx->sim->interactive = 1;
    if(strcmp(buf,"MODE_SUM")==0)   ctx->sim->interactive = 0;
  }
  return NULL;
}

//TODO
//remake so that it sends curRep, Rep, whoel CelStat 1d array to clients
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

  while(*ctx->running) {
    sim_run(ctx->sim, ctx->running);
  }
  sim_save_to_file(ctx->sim);
  *ctx->running = 0;
  return NULL;
}

//argc size 1(file path) + 2(ipc) + 4 (walker) + 4 (world) + 1(trajectory) + 1(viewmode) + 3(simulation)  
int main(int argc, char** argv) {
  atomic_bool running = 1;
  simulation_t sim;
  if (!sim_init(simulation_t *this, walker_t walker, world_t world, int replications, int k, trajectory_t *trajectory, const char *fSavePath)) {

  }

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

