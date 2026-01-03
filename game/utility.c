#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Bool pos_equals(position_t* p1, position_t* p2) {
  return p1->x == p2->x && p1->y == p2->y;
}

void trajectory_init(trajectory_t* t, int k) {
  memset(t, 0, sizeof(*t));
  t->count = 0;
  t->max = k + 1;
  t->positions = malloc(sizeof(position_t) * t->max);
}

void trajectory_add(trajectory_t* t, position_t p) {
  if (t->count >= t->max) {
    perror("Trajectory position out of boundary");
    return;
  }
  t->positions[t->count] = p;
  t->count++;
}

void trajectory_reset(trajectory_t* t) {
  memset(t->positions, 0, sizeof(position_t) * t->max);
  t->count = 0;
}

void trajectory_destroy(trajectory_t* t) {
  free(t->positions);
  t->positions = NULL;
}

double ct_avg_steps(point_statistics_t* ct) {
  return (double) ct->totalSteps / (double) ct->reachedCenter;
}

double ct_reach_center_prob(point_statistics_t* ct, int replications) {
  return (double) ct->reachedCenter / (double) replications;
}
