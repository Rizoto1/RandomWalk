#ifndef WALKER_H
#define WALKER_H

#include "utility.h"

//Walker directional probabilites
typedef struct {
  double up;
  double down;
  double right;
  double left;
} probability_dir_t;

typedef struct {
  position_t pos;
  probability_dir_t prob;
} walker_t;

//walker
_Bool walker_init(walker_t* this, double probUp, double probDown, double probRight, double probLeft);
void walker_destroy(walker_t* this);
void walker_move(walker_t* this, position_t* newPos, const double dir_prob);

//probabilitties
_Bool validate_probabilities(probability_dir_t* prob);
#endif //WALKER_H
