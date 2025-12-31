#ifndef CLIENT_THREADS_H
#define CLIENT_THREADS_H

#include "clientUtil.h"
#include <game/world.h>

void* thread_receive(void* arg);
void* thread_send(void* arg);
void simulation_menu(client_context_t* context);
void createServer(int type,
                  double up, double down, double right, double left,
                  int width, int height, world_type_t worldType, int obstaclePercentage,
                  viewmode_type_t viewMode,
                  int replications, int k, char* savePath);

#endif
