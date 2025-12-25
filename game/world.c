#include "world.h"
#include <stdlib.h>

void w_init(world_t* w, int width, int height, world_type_t worldType) {
  if (width % 2 == 0) {
    width++;
  }
  if (height % 2 ==0) {
    height++;
  }
  w->width = width;
  w->height = height;
  w->worldType = worldType;
  w->obstacles = calloc(width * height, sizeof(char));
}

void w_destroy(world_t* w) {
  free(w->obstacles);
  w->obstacles = NULL;
}

/*
 * if position is in obstacle in the world return 1,
 * otherwise 0
 */
_Bool w_in_obstacle(world_t* w, position_t* p) {
  return w->obstacles[p->y * w->width + p->x] == 1;
}

/*
 * if position is inside world boundaries return 1,
 * otherwise 0
 */
_Bool w_is_inside_boundaries(world_t* w, position_t* p) {
  return p->x >= 0 && p->x <= w->width -1 && p->y >= 0 && p->y <= w->height -1;
}

/*
 *if x or y is outside world boundaries set their position on the other side of the world
 */
void w_normalize(world_t* w, position_t* p) {
  if (p->x < 0) {
    p->x = w->width - 1;
  } else if (p->x >= w->width) {
    p->x = 0;
  }

  if (p->y < 0) {
    p->y = w->height - 1;
  } else if (p->y >= w->height) {
    p->y = 0;
  }
}
