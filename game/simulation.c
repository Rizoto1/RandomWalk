#include "simulation.h"
#include "utility.h"
#include "walker.h"
#include "world.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

void sim_init(simulation_t* this, walker_t walker, world_t world, int replications, int k, trajectory_t* trajectory, const char* fPath) {

  if (!this) {
    perror("Simulation doesnt exist");
    return;
  }

  this->k = k;
  this->replications = replications;
  this->walker = walker;
  this->world = world; 
  this->pointStats = malloc(world.height * world.width * sizeof(point_statistics_t));
  this->trajectory = trajectory;
  if (fPath) {
    this->fSavePath = fPath;
  }
}

void sim_destroy(simulation_t* this) {
  if (!this) {
    perror("Simulation doesnt exist");
    return;
  }
  this->fSavePath = NULL;
  free(this->pointStats);
  this->pointStats = NULL;
}

static void increment_positions(world_t* w, position_t* p) {
  p->x++;
  if (p->x >= w->width) {
    p->x = 0;
    p->y++;
    if (p->y >= w->height) {
      p->y = 0;
    }
  }
}

void sim_run(simulation_t* this) {
  if (!this) {
    perror("Simulation doesnt exist");
    return;
  }

  srand48(time(NULL));
  position_t pos = {0, 0};
  long points = this->world.height * this->world.width;
  for (long rep = 0; rep < this->replications; rep++) {
    pos.x = 0;
    pos.y = 0;
    for (long c = 0; c < points; c++) {
      if (w_in_obstacle(&this->world, &pos)) {
        increment_positions(&this->world, &pos);
        continue;
      }
      this->walker.pos = pos;
      sim_simulate_from(this);
      increment_positions(&this->world, &pos);

    }
  }
}

static point_statistics_t* get_point_statistic(simulation_t* this, position_t* pos) {
  return &this->pointStats[pos->y * this->world.width + pos->x];
}

#define ATTEMPTS 10

/*
 * simulates a single replication.
*/
void sim_simulate_from(simulation_t* this) {
  if (!this) {
    perror("Simulation doesnt exist");
    return;
  }

  position_t centerPos = {(int)floor((double)this->world.width / 2), (int)floor((double)this->world.height / 2)};
  point_statistics_t* cs = get_point_statistic(this, &this->walker.pos);
  position_t newPos = this->walker.pos;
  position_t* p_newPos = &newPos;
  for (int step = 0; step < this->k;) {
    _Bool moveSuccseful = 0;
    int attempts = 0;
    while (!moveSuccseful && attempts < ATTEMPTS) {
      attempts++;
      walker_move(&this->walker, p_newPos, drand48()); 
      if (!p_newPos) {
        perror("Invalid new position");
        return;
      }

      if (!w_is_inside_boundaries(&this->world, p_newPos)) {
        w_normalize(&this->world, p_newPos);
      }
      if (!w_in_obstacle(&this->world, p_newPos)) {
        this->walker.pos = newPos;
        moveSuccseful = 1;
      }
    }

    if (moveSuccseful) {
      step++;
      if (this->trajectory) {
        this->trajectory->positions[this->trajectory->count] = this->walker.pos;
        this->trajectory->count++;
      }

      if (pos_equals(p_newPos, &centerPos)) {
        cs->reachedCenter++;
        cs->totalSteps += step + 1;
        return;
      }
    }
  }
  return;
}

/*
 *
 */
static void deriveWorldFilename(const char* mainFile, char* worldFileOut) {
  char name[256];
  strcpy(name, mainFile);

  char *dot = strrchr(name, '.');
  if (dot != NULL) *dot = '\0';

  sprintf(worldFileOut, "%s_world.txt", name);
}

/*
 *
 */
int sim_load_from_file(simulation_t* this, const char* fPath) {
  if (!this) {
    perror("Simulation load: Invalid simulation");
    return 0;
  }

  FILE* f = fopen(this->fSavePath, "r");
  if (!f) {
    perror("Simulation load: file doesn't exist");
    return 0;
  }
  
  char worldPath[256];
  deriveWorldFilename(fPath, worldPath);
  if (w_load_from_file(&this->world, worldPath) == 0) {
    perror("Simulation load: Failed to load world\n");
    fclose(f);
    return 0;
  }

  double up, down, right, left;
  if (fscanf(f, "%lf %lf %lf %lf\n",
         &up,
         &down,
         &right,
         &left) != 4) {
    perror("Simulation load: improper probabilities.\n");
    fclose(f);
    return 0;
  };

  walker_t walker;
  walker_init(&walker, up, down, right, left);
  this->walker = walker;

  if (fscanf(f, "%d\n", &this->replications) != 1) {
    perror("Simulation load: invalid replicationsqn");
    fclose(f);
    return 0;
  }

  if (fscanf(f, "%d\n", &this->k) != 1) {
    perror("Simulation load: failed to load k\n");
    fclose(f);
    return 0;
  }

  int width = this->world.width;
  int height = this->world.height;

  if (!this->pointStats) {
    this->pointStats = malloc(width * height * sizeof(point_statistics_t));
  }

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      point_statistics_t* cs = get_point_statistic(this, &(position_t){x, y});
      fscanf(f, "%d;%ld", &cs->reachedCenter, &cs->totalSteps);

      if (x < width - 1) {
        fgetc(f);  // odčítať medzeru
      }
    }
    fgetc(f);  // newline \n
  }

  fclose(f);
  return 1;
}

/*
 *This function save simulation to a file. First it tries to save a world, whom save file is derived from fSavePath
 *by adding "_world" to the file name. If saving the world was successful then it proceeds to save simulation related
 *data to fSavePath.
 *If overall saving process was successful returns 1, otherwise 0.
 *
 *Created with help from AI.
 */
int sim_save_to_file(simulation_t* this) {
  if (!this) {
    perror("Invalid simulation\n");
    return 0;
  }

  char worldPath[256];
  deriveWorldFilename(this->fSavePath, worldPath);
  if (w_save_to_file(&this->world, worldPath) == 0) {
    perror("Failed to save simulation: Failed to save world\n");
    return 0;
  }
  FILE* f = fopen(this->fSavePath, "w");
  if (!f) {
    perror("Simulation: file doesn't exist\n");
    return 0;
  }

  fprintf(f, "%f %f %f %f\n",
          this->walker.prob.up, this->walker.prob.down,
          this->walker.prob.right, this->walker.prob.left);
  fprintf(f, "%d\n", this->replications);
  fprintf(f, "%d\n", this->k);

  int width = this->world.width;
  int height = this->world.height;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      point_statistics_t* cs = get_point_statistic(this, &(position_t){x, y});
      fprintf(f, "%d;%ld", cs->reachedCenter, cs->totalSteps);
      if (x < width - 1)
        fprintf(f, " ");
    }
    fprintf(f, "\n"); 
  }

  fclose(f);
  return 1;
}
