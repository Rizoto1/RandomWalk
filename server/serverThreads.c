#include "serverThreads.h"
#include "serverUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

//TODO
//jedno recv thread by malo byť na jedného používateľa
//keďže sa budú pripájať viacerý používateľia, tak musím si uchovávať každého fd
void* server_recv_thread(void* arg) {
  server_ctx_t* ctx = arg;
  char cmd;
  printf("Server: Starting recv thread\n");
  while (atomic_load(ctx->running)) {
    if (socket_recv(ctx->sock, &cmd, sizeof(cmd)) >= 0) {
      switch(cmd) {
        case 's':  ctx->sim->interactive = 1; break;
        case 'u':  ctx->sim->interactive = 0; break;
        case 'q':  atomic_store(ctx->running, 0); break;
      }
    }
  }
  /*char buf[64];

  while(*ctx->running) {
    if(ctx->type==0) pipe_recv(ctx->pipe,buf,64);
    if(ctx->type==1) shm_read(ctx->shm,buf,64);
    if(ctx->type==2) socket_recv(ctx->sock,buf,64);

    if(strcmp(buf,"STOP")==0) *ctx->running = 0;
    if(strcmp(buf,"MODE_INTER")==0) ctx->sim->interactive = 1;
    if(strcmp(buf,"MODE_SUM")==0)   ctx->sim->interactive = 0;
  }*/
  printf("Server: Terminating recv thread\n");
  return NULL;
}

//TODO
//remake so that is sends what user specified based on view mode
void* server_send_thread(void* arg) {
  server_ctx_t* c = arg;
  int size = c->sim->world.width * c->sim->world.height * sizeof(double) * 2;
  double* buf = malloc(size);

  printf("Server: Starting send thread\n");
  while (atomic_load(c->running)) {
    packet_header_t h = { c->sim->currentReplication, c->sim->replications, c->sim->world.width, c->sim->world.height };
    socket_send(c->sock, &h, sizeof(h));

    for (int i=0;i<c->sim->world.width * c->sim->world.height;i++) {
      buf[i*2]   = ct_avg_steps(&c->sim->pointStats[i]);
      buf[i*2+1] = ct_reach_center_prob(&c->sim->pointStats[i], c->sim->replications);
    }
    socket_send(c->sock, buf, size);
    usleep(300000);
  }
  free(buf);
  /*char message[256];

  while(*ctx->running) {
    sprintf(message,"%d/%d",ctx->sim->currentReplication,ctx->sim->replications);

    if(ctx->type==0) pipe_send(ctx->pipe,message);
    if(ctx->type==2) socket_send(ctx->sock,message);
    if(ctx->type==1) shm_write(ctx->shm,&ctx->sim->currentReplication); // 🔁 SHM nám posiela len číslo

    sleep(1);
  }*/
  printf("Server: Terminating send thread\n");
  return NULL;
}

void* simulation_thread(void* arg) {
  server_ctx_t* ctx = arg;

  printf("Server: Starting simulation\n");
  while(*ctx->running) {
    sim_run(ctx->sim, ctx->running);
  }
  printf("Server: simulation finished. Saving to file.\n");
  sim_save_to_file(ctx->sim);
  *ctx->running = 0;
  return NULL;
}
