#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <ipc/ipcSocket.h>
#include "game/simulation.h"
#include "game/utility.h"
#include "game/walker.h"
#include "game/world.h"
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

//argc size 1(file path) + 2(ipc) + 4 (walker) + 4 (world) + 1(viewmode) + 3(simulation)  
int main(int argc, char** argv) {
  if (argc < 15) {
    perror("Server: invalid ammount of parameters");
    return 1;
  }

  double up = strtod(argv[3], NULL);
  double down = strtod(argv[4], NULL);
  double right = strtod(argv[5], NULL);
  double left = strtod(argv[6], NULL);
 
  walker_t walker;
  if (!walker_init(&walker, up, down, right, left)) {
    perror("Server: walker init failed\n");
    return 1;
  }

  world_t world;
  int width = atoi(argv[7]);
  int height = atoi(argv[8]);
  int worldType = atoi(argv[9]);
  int obstaclePercantage = atoi(argv[10]);
  if (!w_init(&world, width, height, (world_type_t)worldType, obstaclePercantage)) {
    perror("Server: world init failed\n");
    return 1;
  }
  
  trajectory_t trajectory;
  trajectory_t* p_trajectory = NULL;
  int viewMode = atoi(argv[11]);
  int k = atoi(argv[13]);

  if (viewMode == INTERACTIVE) {
    trajectory_init(&trajectory, k);
    p_trajectory = &trajectory;
  }

  int replications = atoi(argv[12]);
  atomic_bool running = 1;
  simulation_t sim;
  if (!sim_init(&sim, walker, world, replications, k, p_trajectory, argv[14])) {
    perror("Server: sim init failed\n");
    return 1;
  }

  ServerCtx ctx = {0};
  ctx.running = &running;
  ctx.sim = &sim;

  if(strcmp(argv[1],"pipe")==0) {
    pipe_t p = pipe_init_server(argv[2]);
    ctx.pipe = malloc(sizeof(pipe_t)); *ctx.pipe = p; ctx.type = 0;
  }
  if(strcmp(argv[1],"sock")==0) {
    socket_t s = socket_init_server(atoi(argv[2]));
    ctx.sock = malloc(sizeof(socket_t)); *ctx.sock = s; ctx.type = 2;
  }
  if(strcmp(argv[1],"shm")==0) {
    shm_t s = shm_init_server(atoi(argv[2]), &sim);
    ctx.shm = malloc(sizeof(shm_t)); *ctx.shm = s; ctx.type = 1;
  }

  pthread_t tr, ts, tsim;
  pthread_create(&tr,NULL,server_recv_thread,&ctx);
  pthread_create(&ts,NULL,server_send_thread,&ctx);
  pthread_create(&tsim,NULL,simulation_thread,&ctx);

  pthread_join(tr,NULL);
  pthread_join(ts,NULL);
  pthread_join(tsim,NULL);

  sim_destroy(&sim);
}

