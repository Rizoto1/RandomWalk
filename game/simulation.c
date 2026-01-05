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

/*
 * Initializes simulation.
 * If everything succeds returns 1, otherwise 0.
 */
_Bool sim_init(simulation_t* this, walker_t walker, world_t world, int replications, int k, const char* fLoadPath) {

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
  if (fLoadPath) {
    this->fSavePath = fLoadPath;
    return 1;
  }
  perror("Simulation: path to file doesnt exist");
  return 0;
}

/*
 * Destroys simulation.
 */
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

/*
 * Returns point_statistics_t based on position.
 */
static point_statistics_t* get_point_statistic(simulation_t* this, position_t* pos) {
  return &this->pointStats[pos->y * this->world.width + pos->x];
}

/*
 * Increments postions from {0,0} (down-left) to {y-1,x-1} (top-right)
 */
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

/*
 * As simulation goes it creates trajectory based on walker moves.
 * If it reaches center return 1, otherwise return 0.
 * If error occured return -1.
 */
static int sim_create_trajectory(simulation_t* this) {
  if (!this || !this->trajectory) {
    perror("Simulation doesnt exist");
    return -1;
  }

  position_t center = {(int)floor((double)this->world.width / 2), (int)floor((double)this->world.height / 2)};

  position_t newPos = this->walker.pos;
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

    this->walker.pos = newPos;
    trajectory_add(this->trajectory, newPos);


    return 0;
  }

  return 0;
}

/*
 * Does one full replication.
 */
int sim_run_rep(simulation_t* this) {
  if (!this) return -1;
  
  int result = 0;

  do {
    result = sim_step(this);
  }
  while (result == 0);

  return result;
}

/*
 * Does one step in simulation based on state machine.
 * SIM_INIT_POINT - new point in world. Resets and sets all relevant data.
 * SIM_WALKING - moves walker by one step
 * SIM_NEXT_POINT - trajectory was finished move to next point
 * SIM_NEXT_REPLICATION - one replication was finished start new one
 * SIM_DONE - all replications were done.
 *
 * Returns:
 * -1 - if error occured
 *  0 - when step was succesful
 *  1 - replication was finished
 *  2 - simulation is done
 */
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
 * Creates a new fileName by adding _world to mainFile.
 */
static void deriveWorldFilename(const char* mainFile, char* worldFileOut) {
  char name[256];
  strcpy(name, mainFile);

  char *dot = strrchr(name, '.');
  if (dot != NULL) *dot = '\0';

  sprintf(worldFileOut, "%s_world.txt", name);
}

/*
 * Loads simualtion from file. Firstly it tries to load world from file.
 * If it succeds it continues to load simulation from file.
 * If load was succesful returns 1, otherwise 0.
 *
 * Made with help from AI.
 */
int sim_load_from_file(simulation_t* this, const char* fLoadPath, int replications, const char* fSavePath) {
  if (!this) {
    perror("Simulation load: Invalid simulation");
    return 0;
  }

  FILE* f = fopen(fLoadPath, "r");
  if (!f) {
    perror("Simulation load: file doesn't exist");
    return 0;
  }

  char worldPath[256];
  deriveWorldFilename(fLoadPath, worldPath);
  world_t world;
  if (w_load_from_file(&world, worldPath) == 0) {
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

  int k;
  if (fscanf(f, "%d\n", &k) != 1) {
    perror("Simulation load: failed to load k\n");
    fclose(f);
    return 0;
  }
  
  sim_init(this, walker, world, replications, k, fSavePath);

  fclose(f);
  return 1;
}

/*
 *This function saves simulation to a file. First it tries to save a world, whom save file is derived from fSavePath
 *by adding "_world" to the file name. If saving the world was successful then it proceeds to save simulation related
 *data to fSavePath.
 *If saving process was successful returns 1, otherwise 0.
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
  fprintf(f, "%d\n", this->k);

  int width = this->world.width;
  int height = this->world.height;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      point_statistics_t* cs = get_point_statistic(this, &(position_t){x, y});
      if (w_in_obstacle(&this->world, &(position_t){x,y})) {
        fprintf(f, "%s;%-4s", "XXX.XX\0", "X.XX\0");
      } else {
        fprintf(f, "%06.2f;%.2f", ct_avg_steps(cs) , ct_reach_center_prob(cs, this->replications));
      }
      if (x < width - 1)
        fprintf(f, " ");
    }
    fprintf(f, "\n"); 
  }

  fclose(f);
  return 1;
}
