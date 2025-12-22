#ifndef WALKER_H
#define WALKER_H

#include "utility.h"

//Walker directional probabilites
typedef struct {
  double up;
  double down;
  double left;
  double right;
} probability_dir_t;

typedef struct {
  position_t pos;
  probability_dir_t prob;
} walker_t;

//walker
void walker_init(walker_t* this, int x, int y, double probUp, double probDown, double probRight, double probLeft);
void walker_destroy(walker_t* this);
void walker_move(walker_t* this, movement_dir_t dir);

//probabilitties
_Bool validate_probabilities(probability_dir_t* prob);
#endif //WALKER_H
