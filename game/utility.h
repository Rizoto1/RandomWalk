#ifndef UTILITY_H
#define UTILITY_H

typedef enum {
  UP,
  DOWN,
  RIGHT,
  LEFT,
  MOVE_DIR_COUNT
} movement_dir_t;

typedef enum {
  WO_OBSTACLES,
  W_OBSTALCES
} world_type_t;

typedef struct {
  int x;
  int y;
} position_t;

_Bool pos_equals(position_t* p1, position_t* p2);

typedef struct {
  int count;
  int maxCapacity;
  position_t positions[];
} trajectory_t;

void trajectory_add(trajectory_t* t, position_t p);

typedef struct {
  _Bool reachedCenter;
  long steps;
} trajectory_result_t;

typedef struct {
  int reachedCenter;
  long totalSteps;
} cell_statistics_t;

double ct_avg_steps(cell_statistics_t* ct, int replications);
double ct_reach_center_prob(cell_statistics_t* ct, int replications);
#endif //UTILITY_H
