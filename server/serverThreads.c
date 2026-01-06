#define _GNU_SOURCE  //to find tryjoin
#include <ipc/ipcUtil.h>
#include "serverThreads.h"
#include "serverUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
#include <game/simulation.h>
#include <game/utility.h>
#include <game/walker.h>
#include <game/world.h>
#include <unistd.h>

/*
 * Initializes server from file.
 * If initialization succeeds return 0, otherwise 1.
 */
static int server_load(char** argv, simulation_t* sim) {
  if(!sim_load_from_file(sim, argv[4], atoi(argv[5]), argv[6])) {
    return 1;
  }

  return 0;
}

/*
 * Initializes server based on arguments.
 * If initialization succeeds return 0, otherwise 1.
 */
static int server_create(char** argv, simulation_t* sim) {
  printf("Server: Initializing walker\n");
  double up = strtod(argv[4], NULL);
  double down = strtod(argv[5], NULL);
  double right = strtod(argv[6], NULL);
  double left = strtod(argv[7], NULL);
  walker_t walker;
  if (!walker_init(&walker, up, down, right, left)) {
    perror("Server: walker init failed\n");
    walker_destroy(&walker);
    return 1;
  }

  printf("Server: Initializing world\n");
  world_t world;
  int width = atoi(argv[8]);
  int height = atoi(argv[9]);
  int worldType = atoi(argv[10]);
  int obstaclePercantage = atoi(argv[11]);
  if (!w_init(&world, width, height, (world_type_t)worldType, obstaclePercantage)) {
    perror("Server: world init failed\n");
    walker_destroy(&walker);
    w_destroy(&world);
    return 1;
  }

  //simulation init
  int replications = atoi(argv[12]);
  int k = atoi(argv[13]);
  if (!sim_init(sim, walker, world, replications, k, argv[14])) {
    perror("Server: sim init failed\n");
    walker_destroy(&walker);
    w_destroy(&world);
    return 1;
  }

  return 0;
}

/*
 * Starts the server by creating all the necessary threads.
 * After that tries to join all non-active users threads.
 * When the simulation is finished or the admin terminated the server
 * all users get disconnected and all threads are joined.
 */
static void start(server_ctx_t* ctx) {
  //multithreading
  printf("Sever: starting threads\n");
  pthread_t ts, ta, tsim;
  pthread_create(&ts,NULL,server_send_thread, ctx);
  pthread_create(&ta,NULL,server_accept_thread, ctx);
  pthread_create(&tsim,NULL,simulation_thread, ctx);
  while (atomic_load(ctx->running)) {
    pthread_mutex_lock(&ctx->cManagement.cMutex);
    for (int i = 0; i < ctx->cManagement.maxClients; i++) {
      client_data_t* c = &ctx->cManagement.clients[i];
      if (c->state == CLIENT_TERMINATED && c->tid != 0) {
        int rc = pthread_tryjoin_np(c->tid, NULL);
        if (rc == 0) {
          printf("Server: Joined recv thread\n");
          c->tid = 0;
          remove_client(&ctx->cManagement, i);
        }
      }
    }
    pthread_mutex_unlock(&ctx->cManagement.cMutex);
    usleep(10000);
  }
  
  pthread_cond_signal(&ctx->cManagement.add);

  pthread_mutex_lock(&ctx->cManagement.cMutex);
  for (int i = 0; i < ctx->cManagement.maxClients; i++) {
    client_data_t* c = &ctx->cManagement.clients[i];
    if (c->state == CLIENT_ACTIVE) {
      socket_shutdown(&c->ipc.sock);
      socket_close(&c->ipc.sock);
    }
  }
  pthread_mutex_unlock(&ctx->cManagement.cMutex);

  pthread_join(ts,NULL);
  pthread_join(ta,NULL);
  pthread_join(tsim,NULL);
  pthread_mutex_lock(&ctx->cManagement.cMutex);

  for (int i = 0; i < ctx->cManagement.maxClients; i++) {
    client_data_t* c = &ctx->cManagement.clients[i];

    if (c->state == CLIENT_TERMINATED && atomic_load(&c->active) == 0 && c->tid != 0) {
      pthread_t tid = c->tid;
      c->tid = 0;

      pthread_mutex_unlock(&ctx->cManagement.cMutex);

      printf("Server: Joining recv thread\n");
      pthread_join(tid, NULL);

      pthread_mutex_lock(&ctx->cManagement.cMutex);
      remove_client(&ctx->cManagement, i);
    }
  }

  pthread_mutex_unlock(&ctx->cManagement.cMutex);
}


/*
 * Initializes server base on server load type.
 * 0 - init from file
 * 1 - init based on argv
 * If all initializations succeed and server safely ends return 0, otherwise 1.
 */
int server_init(char** argv) {
  srand(time(NULL));
  simulation_t sim = {0};
  printf("Server: Initializing simulation\n");
  if (atoi(argv[1]) == 0) {
    if (server_load(argv, &sim) == 1) {
      perror("Server: Simulation initialization failed\n");
      sim_destroy(&sim);
      return 1;
    }
  } else {
    if (server_create(argv, &sim) == 1) {
      perror("Server: Simulation initialization failed\n");
      sim_destroy(&sim);
      return 1;
    }
  }

  ipc_ctx_t ipc;
  if (ipc_init(&ipc, 0, argv[2], atoi(argv[3]))) {
    perror("Server: IPC init failed\n");
    walker_destroy(&sim.walker);
    w_destroy(&sim.world);
    sim_destroy(&sim);
    ipc_destroy(&ipc);
    return 1;
  };

  atomic_bool running = 1;
  server_ctx_t ctx = {0};
  int pos;
  if (atoi(argv[1]) == 0) {
    pos = 7;
  } else {
    pos = 15;
  }

  if (server_ctx_init(&ctx, &sim, &running, &ipc, atoi(argv[pos]))) {
    perror("Server: Server init failed\n");
    return 1;
  };

  start(&ctx);

  server_ctx_destroy(&ctx);
  
  sim_destroy(&sim);
  ipc_destroy(ctx.ipc);
  
  return 0;
}

/*
 * This function accepts new clients.
 * If the maxClients is reached it waits for a user to disconnect.
 * Otherwise creates a new user and binds receive thread to him.
 * As an argument it is expecting server_ctx_t.
 * Sets the very first client as isAdmin.
 *
 * Made with help from AI.
 */
void* server_accept_thread(void* arg) {
  server_ctx_t* ctx = arg;

  printf("Server: Starting accept thread\n");
  while (atomic_load(ctx->running)) {
    socket_t s = server_accept_client(ctx->ipc->sock.fd);
    if (s.fd >= 0) {
      printf("Accept fd: %d\n", s.fd);
      client_data_t c;
      memset(&c, 0, sizeof(client_data_t));
      pthread_mutex_lock(&ctx->cManagement.cMutex);
      
      if (ctx->cManagement.clientCount >= ctx->cManagement.maxClients) {
      printf("Accept shutting fd: %d\n", s.fd);
        socket_shutdown(&s);
        socket_close(&s);
          pthread_mutex_unlock(&ctx->cManagement.cMutex);
        continue;
      }

      while (ctx->cManagement.clientCount >= ctx->cManagement.maxClients) {
        printf("Accept waiting\n");
        pthread_cond_wait(&ctx->cManagement.add, &ctx->cManagement.cMutex);

        if (!atomic_load(ctx->running)) {
          pthread_mutex_unlock(&ctx->cManagement.cMutex);
          return NULL;
        }
      }

      if (!atomic_load(ctx->running)) {
        pthread_mutex_unlock(&ctx->cManagement.cMutex);
        return NULL;
      }

      atomic_store(&c.active, 1);
      c.state = CLIENT_ACTIVE;
      c.ipc.sock.fd = s.fd;
      c.sType = AVG_MOVE_COUNT;
      int clientPos = add_client(&ctx->cManagement, c);
      if (clientPos < 0) {
        socket_close(&s);
        pthread_mutex_unlock(&ctx->cManagement.cMutex);
        continue;
      }
      
      if (!ctx->cManagement.adminSet) {
        ctx->cManagement.adminSet = 1;

        ctx->cManagement.clients[clientPos].isAdmin = 1;
      }
      recv_data_t* args = malloc(sizeof(recv_data_t));
      args->clientPos = clientPos;
      args->ctx = ctx;
      pthread_create(&ctx->cManagement.clients[clientPos].tid, NULL, server_recv_thread, args);

      pthread_mutex_unlock(&ctx->cManagement.cMutex);
    }
  }

  printf("Server: Terminating accept thread\n");
  return NULL;
}

/*
 * This function receives data from user. One function is for one client.
 * As an argument it is excepcting recv_data_t.
 * If the client isAdmin then it has permission to shutdown the whole server.
 * Based on certain inputs it changes what will be sent to clients.
 * i - Interactive - shows trajectory of walker
 * s - Summary - Statistical display
 * f - Terminate server, admin/creator only
 * q - Client quits
 * a - Summary 1 - shows average moves to reach center
 * b - Summary 2 - shows probability to reach center
 *
 * Made with help from AI.
 */
void* server_recv_thread(void* arg) {
  recv_data_t* data = arg;
  server_ctx_t* ctx = data->ctx;
  int clientPos = data->clientPos;
  client_data_t* c = &ctx->cManagement.clients[clientPos];
  socket_t sock = c->ipc.sock;

  free(arg);

  char cmd;
  printf("Server: Starting recv thread\n");
  while (atomic_load(ctx->running) && atomic_load(&c->active)) {
    int r = socket_recv(&sock, &cmd, sizeof(cmd));
    if (r <= 0) {
      atomic_store(&c->active,0);
      pthread_mutex_lock(&ctx->cManagement.cMutex);
      c->state = CLIENT_TERMINATED;
      ipc_destroy(&c->ipc);
      pthread_mutex_unlock(&ctx->cManagement.cMutex);
      printf("Server: Terminating recv thread\n");
      return NULL;
    }
    switch(cmd) {
      case 'i':
        pthread_mutex_lock(&ctx->viewMutex);
        ctx->viewMode = INTERACTIVE;
        pthread_mutex_unlock(&ctx->viewMutex);
        break;
      case 's':
        pthread_mutex_lock(&ctx->viewMutex);
        ctx->viewMode = SUMMARY;
        pthread_mutex_unlock(&ctx->viewMutex);
        break;
      case 'f':
        if (c->isAdmin) {
          printf("Server: Shutdown requested by admin\n");
          atomic_store(ctx->running, 0);
        }
        break;
      case 'q':
        atomic_store(&c->active,0);
        pthread_mutex_lock(&ctx->cManagement.cMutex);
        c->state = CLIENT_TERMINATED;
      ipc_destroy(&c->ipc);
        pthread_mutex_unlock(&ctx->cManagement.cMutex);
        printf("Server: Terminating recv thread\n");
        return NULL;

    }
    if (ctx->viewMode == SUMMARY) {
      switch(cmd) {
        case 'a':
          pthread_mutex_lock(&ctx->viewMutex);
          c->sType = AVG_MOVE_COUNT;
          pthread_mutex_unlock(&ctx->viewMutex);
          break;
        case 'b':
          pthread_mutex_lock(&ctx->viewMutex);
          c->sType = PROB_CENTER_REACH;
          pthread_mutex_unlock(&ctx->viewMutex);
          break; 
      }
    }
  }

  atomic_store(&c->active,0);
  pthread_mutex_lock(&ctx->cManagement.cMutex);
  c->state = CLIENT_TERMINATED;
      ipc_destroy(&c->ipc);
  pthread_mutex_unlock(&ctx->cManagement.cMutex);
  printf("Server: Terminating recv thread\n");
  return NULL;
}

/*
 * This function sends interactive type data to all active clients.
 * As an argument it is expecting server_ctx_t.
 *
 * Made with help from AI.
 */
static void send_interactive(void* arg) {
  server_ctx_t* ctx = arg;

  pthread_mutex_lock(&ctx->cManagement.cMutex);
  for (int c = 0; c < ctx->cManagement.maxClients; c++) {
    client_data_t* client = &ctx->cManagement.clients[c];

    if (client->active) {
      pthread_mutex_lock(&ctx->simMutex);

      int size = ctx->sim->world.width * ctx->sim->world.height * sizeof(char);
      char* buf = malloc(size);
      packet_header_t h = {PKT_INTERACTIVE_MAP, ctx->sim->currentReplication, ctx->sim->replications,
        ctx->sim->world.width, ctx->sim->world.height,
        ctx->sim->trajectory->max, ctx->sim->trajectory->count };

      socket_send(&client->ipc.sock, &h, sizeof(h));

      memcpy(buf,
             ctx->sim->world.obstacles,
             size);

      socket_send(&client->ipc.sock, buf, size);
      free(buf);

      size = sizeof(position_t) * ctx->sim->trajectory->max;
      socket_send(&client->ipc.sock, ctx->sim->trajectory->positions, size);
      pthread_mutex_unlock(&ctx->simMutex);
    }
  }

  pthread_mutex_unlock(&ctx->cManagement.cMutex);
}

/*
 * This function sends summary type data to all active clients.
 * As an argument it is expecting server_ctx_t.
 *
 * Made with help from AI.
 */
static void send_summary(void* arg) {
  server_ctx_t* ctx = arg;

  int size = ctx->sim->world.width * ctx->sim->world.height * sizeof(double);
  double* buf = malloc(size);
  packet_header_t h = {PKT_SUMMARY, ctx->sim->currentReplication, ctx->sim->replications,
    ctx->sim->world.width, ctx->sim->world.height,
    0, 0};

  pthread_mutex_lock(&ctx->cManagement.cMutex);
  for (int c = 0; c < ctx->cManagement.maxClients; c++) {
    client_data_t* client = &ctx->cManagement.clients[c];

    pthread_mutex_lock(&ctx->simMutex);
    if (client->active) {
      if (client->sType == AVG_MOVE_COUNT) {

        for (int i=0;i<ctx->sim->world.width * ctx->sim->world.height;i++) {
          if (ctx->sim->world.obstacles[i] == 1) {
            buf[i] = -1;
            continue;
          }

          buf[i] = ct_avg_steps(&ctx->sim->pointStats[i]);
        }

      } else {
        for (int i=0;i<ctx->sim->world.width * ctx->sim->world.height;i++) {
          if (ctx->sim->world.obstacles[i] == 1) {
            buf[i] = -1;
            continue;
          }

          buf[i] = ct_reach_center_prob(&ctx->sim->pointStats[i], ctx->sim->replications);
        }
      }
      socket_send(&client->ipc.sock, &h, sizeof(h));
      socket_send(&client->ipc.sock, buf, size);
    }
    pthread_mutex_unlock(&ctx->simMutex);
  }
  pthread_mutex_unlock(&ctx->cManagement.cMutex);
  free(buf);
}

/*
 * This function sends simulation data to all connected clients in a loop based on server viewMode.
 * If there is no user connected it sets server viewMode to SUMMARY.
 * As an argument it is expecting server_ctx_t.
 */
void* server_send_thread(void* arg) {
  server_ctx_t* ctx = arg;

  printf("Server: Starting send thread\n");
  while (atomic_load(ctx->running)) {

    pthread_mutex_lock(&ctx->viewMutex);
    viewmode_type_t viewMode = ctx->viewMode;
    pthread_mutex_unlock(&ctx->viewMutex); 

    if (ctx->cManagement.clientCount == 0) {
      pthread_mutex_lock(&ctx->viewMutex);
      ctx->viewMode = SUMMARY;
      pthread_mutex_unlock(&ctx->viewMutex); 

      usleep(300000);
      continue;
    }

    if(viewMode == INTERACTIVE) {
      send_interactive(arg);
    } else {
      send_summary(arg);
    }
    usleep(100000);
  }
  printf("Server: Terminating send thread\n");
  return NULL;
}

/*
 * This function runs in a loop and simulates Random Walk until simulation is not finished.
 * When simulation returns 2 it means it has done all replications and running is set to 0.
 * As an argument it is expecting server_ctx_t.
 */
void* simulation_thread(void* arg) {
  server_ctx_t* ctx = arg;
  viewmode_type_t viewMode;
  int result;

  printf("Server: Starting simulation\n");
  while(atomic_load(ctx->running)) {
    pthread_mutex_lock(&ctx->viewMutex);
    viewMode = ctx->viewMode;

    pthread_mutex_lock(&ctx->simMutex);
    if (viewMode == SUMMARY) {
      result = sim_run_rep(ctx->sim);
    } else {
      result = sim_step(ctx->sim);
    }
    pthread_mutex_unlock(&ctx->viewMutex);
    pthread_mutex_unlock(&ctx->simMutex);

    if (result == 2) {
      atomic_store(ctx->running, 0);
    }
    if (viewMode == SUMMARY) {
      usleep(SLEEP_SUMMARY);
    } else {
      usleep(SLEEP_INTERACTIVE);
    }
  }
  printf("Server: simulation finished. Saving to file.\n");
  sim_save_to_file(ctx->sim);
  return NULL;
}
