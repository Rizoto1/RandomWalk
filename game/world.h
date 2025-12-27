#ifndef WORLD_H
#define WORLD_H

#include "utility.h"
#include <stdio.h>

typedef enum {
  WO_OBSTACLES,
  W_OBSTALCES
} world_type_t;

typedef struct {
  int height;
  int width;
  char* obstacles;
  world_type_t worldType;
} world_t;

void w_init(world_t* this, int width, int height, world_type_t worldType);
void w_destroy(world_t* this);
_Bool w_in_obstacle(world_t* this, position_t* p);
_Bool w_is_inside_boundaries(world_t* this, position_t* p);
void w_normalize(world_t* this, position_t* p);
//TODO
//implement
void w_create_obstacles(world_t* this, double obstaclePercantage);
int w_load_from_file(world_t* this, const char* f);
int w_save_to_file(world_t* this, const char* f);
static void w_all_nodes_reachable(world_t* this);

#endif //WORLD_H
