#ifndef SIMULATION_H
#define SIMULATION_H

#include "utility.h"
#include "walker.h"
#include "world.h"

typedef struct {
  int k; //number of steps walker can make
  int replications;
  point_statistics_t* pointStats;
  walker_t walker;
  world_t world;
  _Bool interactive;
  trajectory_t* trajectory;
  const char* fSavePath;
} simulation_t;

void sim_init(simulation_t* this, walker_t walker, world_t world, int replications, int k, trajectory_t* trajectory, const char* fSavePath);
void sim_destroy(simulation_t* this);
void sim_run(simulation_t* this);
void sim_simulate_from(simulation_t* this);
int sim_load_from_file(simulation_t * this, const char* fPath);
int sim_save_to_file(simulation_t* this);

#endif //SIMULATION_H
