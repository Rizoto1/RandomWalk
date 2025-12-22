#include "utility.h"
#include <stdio.h>
_Bool pos_equals(position_t* p1, position_t* p2) {
  return p1->x == p2->x && p1->y == p2->y;
}

void trajectory_add(trajectory_t* t, position_t p) {
  if (t->count >= t->maxCapacity) {
    perror("Trajectory position out of boundary");
    return;
  }
  t->positions[t->count] = p;
  t->count++;
}

double ct_avg_steps(cell_statistics_t* ct, int replications) {
  return (double) ct->totalSteps / (double) replications;
}

double ct_reach_center_prob(cell_statistics_t* ct, int replications) {
  return (double) ct->reachedCenter / (double) replications;
}
