#include "simulation.h"
#include "utility.h"
#include "walker.h"
#include "world.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void sim_init(simulation_t* this, walker_t walker, world_t world, int replications, int k, trajectory_t* trajectory) {
  if (!this) {
    perror("Simulation doesnt exist");
    return;
  }

  this->k = k;
  this->replications = replications;
  this->walker = walker;
  this->world = world; 
  this->cellStats = malloc(world.height * world.width * sizeof(cell_statistics_t));
  this->trajectory = trajectory;
}

void sim_destroy(simulation_t* this) {
  if (!this) {
    perror("Simulation doesnt exist");
    return;
  }

  free(this->cellStats);
  this->cellStats = NULL;
}

static void increment_positions(world_t* w, position_t* p) {
  if (p->x > w->width) {
    p->x = 0;
    p->y++;
  } else {
    p->x++;
  }
}

void sim_run(simulation_t* this) {
  if (!this) {
    perror("Simulation doesnt exist");
    return;
  }

  srand48(time(NULL));
  position_t pos = {0, 0};
  long cells = this->world.height * this->world.width;
  for (long rep = 0; rep < this->replications; rep++) {
    pos.x = 0;
    pos.y = 0;
    for (long c = 0; c < cells; c++) {
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

static cell_statistics_t* get_cell_statistic(simulation_t* this, position_t* pos) {
  return &this->cellStats[pos->y * this->world.width + pos->x];
}

#define ATTEMPTS 10

/*
 * simulates a single replication.
 * If there is error returns 2.
 * If walker reached center returns 1, otherwise 0.
*/

void sim_simulate_from(simulation_t* this) {
  if (!this) {
    perror("Simulation doesnt exist");
    return;
  }

  position_t centerPos = {0,0};
  cell_statistics_t* cs = get_cell_statistic(this, &this->walker.pos);
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
