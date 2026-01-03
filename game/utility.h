#ifndef UTILITY_H
#define UTILITY_H

//converts 2d into 1d
#define IDX(x, y, w) ((y) * (w) + (x))

typedef enum {
  UP,
  DOWN,
  RIGHT,
  LEFT
} movement_dir_t;

typedef enum {
  INTERACTIVE,
  SUMMARY
} viewmode_type_t;

typedef enum {
  AVG_MOVE_COUNT,
  PROB_CENTER_REACH
} summary_type_t;

typedef struct {
  int x;
  int y;
} position_t;

_Bool pos_equals(position_t* p1, position_t* p2);

typedef struct {
  int count;
  int max;
  position_t* positions;
} trajectory_t;

void trajectory_init(trajectory_t* t, int k);
void trajectory_add(trajectory_t* t, position_t p);
void trajectory_reset(trajectory_t* t);
void trajectory_destroy(trajectory_t* t);

typedef struct {
  int reachedCenter;
  long totalSteps;
} point_statistics_t;

double ct_avg_steps(point_statistics_t* ct);
double ct_reach_center_prob(point_statistics_t* ct, int replications);
#endif //UTILITY_H
