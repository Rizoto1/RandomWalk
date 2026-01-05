#ifndef SIMULATION_H
#define SIMULATION_H

#include "utility.h"
#include "walker.h"
#include "world.h"
#include <stdatomic.h>

#define ATTEMPTS 10

typedef enum {
    SIM_INIT_POINT,
    SIM_WALKING,
    SIM_NEXT_POINT,
    SIM_NEXT_REPLICATION,
    SIM_DONE
} sim_state_t;

typedef struct {
  int k; //number of steps walker can make
  int replications;
  int currentReplication;
  sim_state_t state;
  point_statistics_t* pointStats;
  walker_t walker;
  world_t world;
  position_t startingPoint;
  int pointsStarted;
  trajectory_t* trajectory;
  const char* fSavePath;
} simulation_t;

_Bool sim_init(simulation_t* this, walker_t walker, world_t world, int replications, int k, const char* fSavePath);
void sim_destroy(simulation_t* this);
int sim_step(simulation_t* this);
int sim_run_rep(simulation_t* this);
int sim_load_from_file(simulation_t* this, const char* fLoadPath, int replications, const char* fSavePath);
int sim_save_to_file(simulation_t* this);

#endif //SIMULATION_H
