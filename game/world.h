#ifndef WORLD_H
#define WORLD_H

#include "utility.h"

//TODO
//implement loading from file
//implement obstacle creation
// - algorithm to check if [0,0] is reachable from every non obstacle point

typedef struct {
  int height;
  int width;
  char* obstacles;
  world_type_t worldType;
} world_t;

void w_init(world_t* w, int width, int height, world_type_t worldType);
void w_destroy(world_t* w);
_Bool w_in_obstacle(world_t* w, position_t* p);
_Bool w_is_inside_boundaries(world_t* w, position_t* p);
void w_normalize(world_t* w, position_t* p);
//TODO
//implement
void w_create_obstacles(world_t* w, int obstaclePercantage);
void w_load_world_from_file(world_t* w, const char* fName);
static void w_all_nodes_reachable(world_t* w);

#endif //WORLD_H
