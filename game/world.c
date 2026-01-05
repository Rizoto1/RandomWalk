#include "world.h"
#include "utility.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

//TODO
//divide obstacle percentage to better represent the percentage, because now 50% is like 90% and 80 is 100%

/*
 * This function goes searches the whole map using BFS. 
 * If some non obstacles points have visited attribute set to 0 it means they are unreachable.
 * For simplicity reasons these points are replaced as an obstacle.
 *
 * Created with help from AI.
 */
static void w_all_nodes_reachable(world_t* this) {
  if (!this) return;

  int width = this->width;
  int height = this->height;
  position_t centerPos = {(int)floor((double)this->width / 2), (int)floor((double)this->height / 2)};

  _Bool **visited = malloc(height * sizeof(_Bool*));
  for (int y = 0; y < height; y++) {
    visited[y] = calloc(width, sizeof(_Bool));
  }

  // BFS from center
  position_t* queue = malloc(width * height * sizeof(position_t));
  int head = 0, tail = 0;

  if (!w_in_obstacle(this, &centerPos)) {
    visited[centerPos.y][centerPos.x] = 1;
    queue[tail++] = centerPos;
  }

  while (head < tail) {
    position_t p = queue[head++];
    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (int i = 0; i < 4; i++) {
      int nx = p.x + dirs[i][0];
      int ny = p.y + dirs[i][1];
      if (nx >= 0 && nx < width &&
        ny >= 0 && ny < height &&
        !visited[ny][nx] &&
        !w_in_obstacle(this, &(position_t){nx,ny}))
      {
        visited[ny][nx] = 1;
        queue[tail++] = (position_t){nx, ny};
      }
    }
  }

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (!visited[y][x]) {
        this->obstacles[y * width + x] = 1;
      }
    }
  }

  // free memory
  for (int y = 0; y < height; y++)
    free(visited[y]);
  free(visited);
  free(queue);
}

_Bool w_init(world_t* this, int width, int height, world_type_t worldType, int obstaclePercantage) {
  if (!this) return 0;

  if (width % 2 == 0) {
    width++;
  }
  if (height % 2 == 0) {
    height++;
  }
  this->width = width;
  this->height = height;
  this->worldType = worldType;
  this->obstacles = calloc(width * height, sizeof(char));
  
  if (worldType == W_OBSTALCES) {
    w_create_obstacles(this, (double)obstaclePercantage / 100);
    w_all_nodes_reachable(this);
  }

  return 1;
}

void w_destroy(world_t* this) {
  if (!this) return;

  free(this->obstacles);
  this->obstacles = NULL;
}


/*
 * if position is in obstacle in the world return 1,
 * otherwise 0
 */
_Bool w_in_obstacle(world_t* this, position_t* p) {
  if (!this || !p) return 1;

  return this->obstacles[p->y * this->width + p->x] == 1;
}

/*
 * if position is inside world boundaries return 1,
 * otherwise 0
 */
_Bool w_is_inside_boundaries(world_t* this, position_t* p) {
  if (!this || !p) return 1;

  return p->x >= 0 && p->x < this->width && p->y >= 0 && p->y < this->height;
}

/*
 * if x or y is outside world boundaries set their position on the other side of the world
 */
void w_normalize(world_t* this, position_t* p) {
  if (!this || !p) return;

  if (p->x < 0) {
    p->x = this->width - 1;
  } else if (p->x >= this->width) {
    p->x = 0;
  }

  if (p->y < 0) {
    p->y = this->height - 1;
  } else if (p->y >= this->height) {
    p->y = 0;
  }
}


/*
 * This function creates obstacles based on percentage.
 * If percentage is 20% that means 20% of map will be covered in obstacles.
 *
 * Created with help from AI.
 */
void w_create_obstacles(world_t* this, double obstaclePercantage) {
  if (!this) return;

  int totalCells = this->width * this->height;
  int toBlock = (int)(totalCells * obstaclePercantage);

  position_t centerPos = {(int)floor((double)this->width / 2), (int)floor((double)this->height / 2)};

  srand(time(NULL));

  while (toBlock > 0) {
    int x = rand() % this->width;
    int y = rand() % this->height;

    // skip center
    if (x == centerPos.x && y == centerPos.y) continue;

    int idx = IDX(x, y, this->width);
      if (this->obstacles[idx] == 0) {
        this->obstacles[idx] = 1;
        toBlock--;
      }
  }

}

/*
 * Load world from file.
 * If loaded succesfully returns 1, otherwise 0.
 *
 * Created with help from AI.
 */
int w_load_from_file(world_t* this, const char* fPath) {
  if (!this || !fPath) return 0;

  FILE* f = fopen(fPath, "r");
  if (!f) {
    perror("Invalid file: invalid file path\n");
    return 0;
  }

  int width, height, typeNum;

  // Load width & height
  if (fscanf(f, "%d %d", &width, &height) != 2) {
    perror("Invalid file: missing width/height\n");
    fclose(f);
    return 0;
  }

  // Load world type
  if (fscanf(f, "%d", &typeNum) != 1) {
    perror("Invalid file: missing world type\n");
    fclose(f);
    return 0;
  }

  // Initialize world
  w_init(this, width, height, WO_OBSTACLES, 0);

  this->worldType = (world_type_t)typeNum;

  if (typeNum == 0) {
    // calloc already zeroed obstacles – nothing to read
    fclose(f);
    return 1;
  } 

  fgetc(f); // consume newline after the number

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int c = fgetc(f);
      if (c == EOF) {
        perror("Invalid file: unexpected EOF\n");
        fclose(f);
        return 0;
      }
      if (c == '1')      this->obstacles[IDX(x,y,width)] = 1;
      else if (c == '0') this->obstacles[IDX(x,y,width)] = 0;
      else {
        fprintf(stderr, "Invalid char '%c' at %d,%d – expected '0' or '1'\n",
                        c, x, y);
        fclose(f);
        return 0;
      }
    }
    fgetc(f); // skip newline
  }

  fclose(f);
  w_all_nodes_reachable(this);

  return 1;
}

/*
 * This function save a world to a file.
 * If it succeeds returns 1 otherwise 0.
 *
 * Creat with help from AI.
 */
int w_save_to_file(world_t* this, const char* fPath) {
  if (!this || !fPath) return 0; 
  FILE *f = fopen(fPath, "w");
  if (!f) {
    perror("Invalid file: fopen failed");
    return 0;
  }

  // Save size and type
  fprintf(f, "%d %d\n", this->width, this->height);
  fprintf(f, "%d\n", this->worldType);

  // If world has no obstacles -> stop
  if (this->worldType == WO_OBSTACLES) {
    fclose(f);
    return 1;
  }

  // Save obstacle map
  for (int y = 0; y < this->height; y++) {
    for (int x = 0; x < this->width; x++) {
      fprintf(f, "%d", this->obstacles[IDX(x,y,this->width)]);
    }
    fprintf(f, "\n");
  }

  fclose(f);
  return 1;
}

