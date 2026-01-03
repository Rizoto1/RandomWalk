#include "simulation.h"
#include "utility.h"
#include "walker.h"
#include "world.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

//TODO
//for some reason sim_step gets stuck at center point and never goes further unless I change to summary
_Bool sim_init(simulation_t* this, walker_t walker, world_t world, int replications, int k, const char* fPath) {

  if (!this) {
    perror("Simulation doesnt exist");
    return 0;
  }
  srand48(time(NULL));

  this->k = k;
  this->replications = replications;
  this->walker = walker;
  this->world = world;
  this->pointStats = calloc(world.height * world.width, sizeof(point_statistics_t));
  this->currentReplication = 0;
  this->trajectory = malloc(sizeof(trajectory_t));
  this->startingPoint = (position_t){0, 0};
  this->pointsStarted = 0;
  this->state = SIM_INIT_POINT;
  trajectory_init(this->trajectory, k);
  if (fPath) {
    this->fSavePath = fPath;
    return 1;
  }
  perror("Simulation: path to file doesnt exist");
  return 0;
}

void sim_destroy(simulation_t* this) {
  if (!this) {
    perror("Simulation doesnt exist");
    return;
  }
  this->fSavePath = NULL;
  free(this->pointStats);
  this->pointStats = NULL;
  trajectory_destroy(this->trajectory);
  free(this->trajectory);
  this->trajectory =NULL;
}

static point_statistics_t* get_point_statistic(simulation_t* this, position_t* pos) {
  return &this->pointStats[pos->y * this->world.width + pos->x];
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

/*/*
 * simulates a single replication.
*/
/*
static void sim_create_trajectory(simulation_t* this, int max) {
  if (!this) {
    perror("Simulation doesnt exist");
    return;
  }

  position_t centerPos = {(int)floor((double)this->world.width / 2), (int)floor((double)this->world.height / 2)};
  point_statistics_t* cs = get_point_statistic(this, &this->walker.pos);
  position_t newPos = this->walker.pos;
  position_t* p_newPos = &newPos;
  for (; this->trajectory->count < max;) {
    _Bool moveSuccseful = 0;
    int attempts = 0;
    while (!moveSuccseful && attempts < ATTEMPTS) {
      if (pos_equals(p_newPos, &centerPos)) {
        cs->reachedCenter++;
        cs->totalSteps = cs->totalSteps + this->trajectory->count - 1;
        trajectory_add(this->trajectory, centerPos);
        this->trajectory->count = 0;
        return;
      }
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
      trajectory_add(this->trajectory, this->walker.pos);
    }
  }
  return;
}
*/

static int sim_create_trajectory(simulation_t* this) {
  if (!this || !this->trajectory) {
    perror("Simulation doesnt exist");
    return -1;
  }

  position_t center = {(int)floor((double)this->world.width / 2), (int)floor((double)this->world.height / 2)};

  position_t newPos = this->walker.pos;
    /* dosiahnutý stred */
    if (pos_equals(&newPos, &center)) {
      point_statistics_t* cs =
        get_point_statistic(this, &this->startingPoint);

      cs->reachedCenter++;
      cs->totalSteps = cs->totalSteps + this->trajectory->count - 1;

      return 1;
    }
  int attempts = 0;

  while (attempts++ < ATTEMPTS) {

    walker_move(&this->walker, &newPos, drand48());

    if (!w_is_inside_boundaries(&this->world, &newPos)) {
      w_normalize(&this->world, &newPos);
    }

    if (w_in_obstacle(&this->world, &newPos)) {
      continue;
    }

    /* úspešný krok */
    this->walker.pos = newPos;
    trajectory_add(this->trajectory, newPos);


    return 0;
  }

  /* nepodarilo sa pohnúť (zriedkavé) */
  return 0;
}


int sim_run_rep(simulation_t* this) {
  if (!this) return -1;
  
  int result = 0;

  do {
    result = sim_step(this);
  }
  while (result == 0);

  return result;
}


int sim_step(simulation_t* s) {
  if (!s) return -1;

  long totalPoints = s->world.width * s->world.height;

  switch (s->state) {

    case SIM_INIT_POINT:
      while (w_in_obstacle(&s->world, &s->startingPoint)) {
        increment_positions(&s->world, &s->startingPoint);
        s->pointsStarted++;

        if (s->pointsStarted >= totalPoints) {
          s->state = SIM_NEXT_REPLICATION;
          return 0;
        }
      }
      if (s->pointsStarted >= totalPoints) {
        s->state = SIM_NEXT_REPLICATION;
        return 0;
      }

      trajectory_reset(s->trajectory);
      trajectory_add(s->trajectory, s->startingPoint);
      s->walker.pos = s->startingPoint;
      s->state = SIM_WALKING;
      return 0;

    case SIM_WALKING:
      if (sim_create_trajectory(s) || s->trajectory->count >= s->k) {
        s->state = SIM_NEXT_POINT;
      }
      return 0;

    case SIM_NEXT_POINT:
      s->pointsStarted++;
      increment_positions(&s->world, &s->startingPoint);
      s->state = SIM_INIT_POINT;
      return 0;

    case SIM_NEXT_REPLICATION:
      s->currentReplication++;
      if (s->currentReplication >= s->replications) {
        s->state = SIM_DONE;
      } else {
        s->pointsStarted = 0;
        s->startingPoint = (position_t){0,0};
        s->state = SIM_INIT_POINT;
      }
      return 1;

    case SIM_DONE:
      return 2;
  }

  return 0;
}


/*
int sim_run_rep(simulation_t* this) {
  if (!this) {
    perror("Simulation doesnt exist");
    return -1;
  }
  //if the whole trajectory was filled (the starting point trajectory was completed)
  if (this->trajectory->count >= this->k) {
    this->pointsStarted++;
    this->trajectory->count = 0;
    memset(this->trajectory->positions, 0, sizeof(position_t) * this->trajectory->max);
    increment_positions(&this->world, &this->startingPoint);
  }

  long points = this->world.height * this->world.width;

  for (; this->pointsStarted < points;) {
    while (w_in_obstacle(&this->world, &this->startingPoint)) {
      this->pointsStarted++;
      increment_positions(&this->world, &this->startingPoint);
    }

    //if the trajectory is reset
    if (this->trajectory->count == 0) {
      trajectory_add(this->trajectory, this->startingPoint);
    }

    this->pointsStarted++;
    this->walker.pos = this->trajectory->positions[this->trajectory->count - 1];
    sim_create_trajectory(this, this->k);
    memset(this->trajectory->positions, 0, sizeof(position_t) * this->trajectory->max);
    this->trajectory->count = 0;
    increment_positions(&this->world, &this->startingPoint);

  }
  this->currentReplication++;
  this->pointsStarted = 0;
  this->startingPoint = (position_t){0,0};
  memset(this->trajectory->positions, 0, sizeof(position_t) * this->trajectory->max);

  if (this->currentReplication == this->replications) {
    return 1;
  }

  return 0;
}
*/
/*
int sim_step(simulation_t* this) {
  if (!this) {
    perror("Simulation doesnt exist");
    return -1;
  }

  //if the whole trajectory was filled (the starting point trajectory was completed)
  if (this->trajectory->count >= this->k) {
    this->pointsStarted++;
    this->trajectory->count = 0;
    memset(this->trajectory->positions, 0, sizeof(position_t) * this->trajectory->max);
    increment_positions(&this->world, &this->startingPoint);
  }

  long points = this->world.height * this->world.width;

  //if the whole map was completed (replication started from each non obstacle point)
  if (this->pointsStarted == points) {
    this->currentReplication++;
    this->pointsStarted = 0;
    memset(this->trajectory->positions, 0, sizeof(position_t) * this->trajectory->max);
    this->startingPoint = (position_t){0,0};
  }

  if (this->currentReplication == this->replications) {
    return 1;
  }

  //while there is obstacle in new starting position go to next
  while (w_in_obstacle(&this->world, &this->startingPoint)) {
    this->pointsStarted++;
    increment_positions(&this->world, &this->startingPoint);
  }
  //if the trajectory is reset
  if (this->trajectory->count == 0) {
    trajectory_add(this->trajectory, this->startingPoint);
  }

  this->walker.pos = this->trajectory->positions[this->trajectory->count - 1];
  sim_create_trajectory(this, this->trajectory->count + 1);

  return 0;

}
*/
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
