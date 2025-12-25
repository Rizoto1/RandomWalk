#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
_Bool pos_equals(position_t* p1, position_t* p2) {
  return p1->x == p2->x && p1->y == p2->y;
}

void trajectory_init(trajectory_t* t, int k) {
  if (!t) return;
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

void trajectory_destroy(trajectory_t* t) {
  free(t->positions);
  t->positions = NULL;
}

double ct_avg_steps(cell_statistics_t* ct, int replications) {
  return (double) ct->totalSteps / (double) replications;
}

double ct_reach_center_prob(cell_statistics_t* ct, int replications) {
  return (double) ct->reachedCenter / (double) replications;
}
