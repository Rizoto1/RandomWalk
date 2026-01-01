#include "serverThreads.h"
#include "serverUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
#include <ipc/ipcPipe.h>
#include <ipc/ipcShmSem.h>
#include <ipc/ipcSocket.h>
#include <game/simulation.h>
#include <game/utility.h>
#include <game/walker.h>
#include <game/world.h>
#include <unistd.h>

_Bool server_init(char** argv) {
  //walker init
  double up = strtod(argv[3], NULL);
  double down = strtod(argv[4], NULL);
  double right = strtod(argv[5], NULL);
  double left = strtod(argv[6], NULL);

  printf("Server: Initializing walker\n");
  walker_t walker;
  if (!walker_init(&walker, up, down, right, left)) {
    perror("Server: walker init failed\n");
    return 1;
  }

  //world init
  printf("Server: Initializing world\n");
  world_t world;
  int width = atoi(argv[7]);
  int height = atoi(argv[8]);
  int worldType = atoi(argv[9]);
  int obstaclePercantage = atoi(argv[10]);
  if (!w_init(&world, width, height, (world_type_t)worldType, obstaclePercantage)) {
    perror("Server: world init failed\n");
    return 1;
  }

  //simulation init
  printf("Server: Initializing simulation\n");
  int replications = atoi(argv[12]);
  int k = atoi(argv[13]);
  atomic_bool running = 1;
  simulation_t sim;
  if (!sim_init(&sim, walker, world, replications, k, argv[14])) {
    perror("Server: sim init failed\n");
    return 1;
  }

  //creating IPC protocol
  server_ctx_t ctx = {0};
  ctx.running = &running;
  ctx.sim = &sim;
  printf("Server: creating connection\n");
  /*if(strcmp(argv[1],"pipe")==0) {
    pipe_t p = pipe_init_server(argv[2]);
    ctx.pipe = malloc(sizeof(pipe_t)); *ctx.pipe = p; ctx.type = 0;
  }*/
  if(strcmp(argv[1],"sock")==0) {
    socket_t s = socket_init_server(atoi(argv[2]));
    if (s.fd < 0) {
      perror("Server: creating connection failed. Terminating server\n");
      return 1;
    }
    ctx.sock = malloc(sizeof(socket_t));
    *ctx.sock = s;
    ctx.type = 2;
  }
  /*if(strcmp(argv[1],"shm")==0) {
    shm_t s = shm_init_server(atoi(argv[2]), &sim);
    ctx.shm = malloc(sizeof(shm_t)); *ctx.shm = s; ctx.type = 1;
  }*/

  //multithreading
  printf("Sever: starting threads\n");
  pthread_t tr, ta, tsim;
  pthread_create(&tr,NULL,server_recv_thread,&ctx);
  pthread_create(&ta,NULL,server_accept_thread,&ctx);
  pthread_create(&tsim,NULL,simulation_thread,&ctx);

  pthread_join(tr,NULL);
  pthread_join(ta,NULL);
  pthread_join(tsim,NULL);
  sim_destroy(&sim);
  free(ctx.sock);
  walker_destroy(&walker);
  w_destroy(&world);

  return 0;
}

//maybe it will throw errors if i send like -1 to socket_close
void* server_accept_thread(void* arg) {
  server_ctx_t* ctx = arg;

  printf("Server: Starting accept thread\n");
  while (atomic_load(ctx->running)) {
    socket_t s = server_accept_client(ctx->sock->fd);
    if (s.fd > 0) {
      client_data_t c = {0};
      pthread_mutex_lock(&ctx->cManagement.cMutex);

      while (ctx->cManagement.clientCount >= SERVER_CAPACITY) {
        pthread_cond_wait(&ctx->cManagement.add, &ctx->cManagement.cMutex);
      }
      c.id = ctx->cManagement.idCounter;
      ctx->cManagement.idCounter++;
      c.active = 1;
      c.socket.fd = s.fd;
      c.sType = AVG_MOVE_COUNT;
      int clientPos = ctx->cManagement.clientCount;
      add_client(&ctx->cManagement, c);
      pthread_mutex_unlock(&ctx->cManagement.cMutex);
      pthread_cond_broadcast(&ctx->cManagement.remove);
    
      pthread_t tid;
      recv_data_t* args = malloc(sizeof(recv_data_t));
      args->clientPos = clientPos;
      args->ctx = arg;
      pthread_create(&tid, NULL, server_recv_thread, args);
      pthread_detach(tid);
      
    }
  }
  printf("Server: Terminating accept thread\n");
  return NULL;
}

void* server_recv_thread(void* arg) {
  recv_data_t* data = arg;
  server_ctx_t* ctx = data->ctx;
  int clientPos = data->clientPos;
  client_data_t* c = &ctx->cManagement.clients[clientPos];
  socket_t sock = c->socket;

  free(arg);

  char cmd;
  printf("Server: Starting recv thread\n");
  while (atomic_load(ctx->running) && c->active) {
    int r = socket_recv(&sock, &cmd, sizeof(cmd));
    if (r <= 0) {
      pthread_mutex_lock(&ctx->cManagement.cMutex);
      remove_client(&ctx->cManagement, clientPos);
      pthread_mutex_unlock(&ctx->cManagement.cMutex);
      break;
    }
      pthread_mutex_lock(&ctx->vMutex);
      switch(cmd) {
        case 'i':  ctx->viewMode = INTERACTIVE; pthread_mutex_unlock(&ctx->vMutex); break;
        case 's':  ctx->viewMode = SUMMARY; pthread_mutex_unlock(&ctx->vMutex); break;
        case 'q':  atomic_store(ctx->running, 0); pthread_mutex_unlock(&ctx->vMutex); break;
      }
    if (ctx->viewMode == SUMMARY) {
      switch(cmd) {
        case 'a': c->sType = AVG_MOVE_COUNT; break;
        case 'b': c->sType = PROB_CENTER_REACH; break; 
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

static void send_interactive(void* arg) {
  server_ctx_t* ctx = arg;

  int size = ctx->sim->world.width * ctx->sim->world.height * sizeof(double);
  double* buf = malloc(size);
  packet_header_t h = { ctx->sim->currentReplication, ctx->sim->replications, ctx->sim->world.width, ctx->sim->world.height };

  pthread_mutex_lock(&ctx->cManagement.cMutex);
  for (int c = 0; c < SERVER_CAPACITY; c++) {
    client_data_t* client = &ctx->cManagement.clients[c];
    if (client->active) {
      socket_send(&client->socket, &h, sizeof(h));
      for (int i=0;i<ctx->sim->world.width * ctx->sim->world.height;i++) {
        buf[i]   = ct_avg_steps(&ctx->sim->pointStats[i]);
      }
      socket_send(ctx->sock, buf, size);
    }
  }

  pthread_mutex_unlock(&ctx->cManagement.cMutex);
  free(buf);

}

static void send_summary(void* arg) {
  server_ctx_t* ctx = arg;

  int size = ctx->sim->world.width * ctx->sim->world.height * sizeof(double);
  double* buf = malloc(size);
  packet_header_t h = { ctx->sim->currentReplication, ctx->sim->replications, ctx->sim->world.width, ctx->sim->world.height };

  pthread_mutex_lock(&ctx->cManagement.cMutex);
  for (int c = 0; c < SERVER_CAPACITY; c++) {
    client_data_t* client = &ctx->cManagement.clients[c];
    if (client->active && client->sType == AVG_MOVE_COUNT) {
      socket_send(&client->socket, &h, sizeof(h));
      for (int i=0;i<ctx->sim->world.width * ctx->sim->world.height;i++) {
        buf[i]   = ct_avg_steps(&ctx->sim->pointStats[i]);
      }
      socket_send(ctx->sock, buf, size);

    } else if (client->active && client->sType == PROB_CENTER_REACH) {
      socket_send(&client->socket, &h, sizeof(h));
      for (int i=0;i<ctx->sim->world.width * ctx->sim->world.height;i++) {
        buf[i*2] = ct_reach_center_prob(&ctx->sim->pointStats[i], ctx->sim->replications);
      }
      socket_send(ctx->sock, buf, size);

    }
  }

  pthread_mutex_unlock(&ctx->cManagement.cMutex);
  free(buf);
}

//TODO
//remake so that is sends what user specified based on view mode
void* server_send_thread(void* arg) {
  server_ctx_t* ctx = arg;

  printf("Server: Starting send thread\n");
  while (atomic_load(ctx->running)) {

    pthread_mutex_lock(&ctx->vMutex);
    viewmode_type_t viewMode = ctx->viewMode;
    pthread_mutex_unlock(&ctx->vMutex); 

    if(viewMode == INTERACTIVE) {
      send_interactive(arg);
    } else {
      send_summary(arg);
    }
    usleep(300000);
  }
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
