#include "walker.h"
#include "utility.h"
#include <stdio.h>
#include <string.h>

_Bool walker_init(walker_t* this, double probUp, double probDown, double probRight, double probLeft) {
  if (!this) return 0;

  position_t pos = {0, 0};
  probability_dir_t prob = {probUp, probDown, probRight, probLeft};
  if (!validate_probabilities(&prob)) {
    perror("Sum of directional probabilites is not 1");
    return 0;
  }
  this->pos = pos;
  this->prob = prob;

  return 1;
}

void walker_destroy(walker_t* this) {
  if(!this) return;
  memset(this, 0, sizeof(*this));
}

typedef struct {
  int dx;
  int dy;
} dir_delta_t;

static const dir_delta_t DIR_DELTAS[] = {
  {0, 1},   //up
  {0, -1},  //down
  {1, 0},   //right
  {-1, 0}   //left
};

static movement_dir_t probability_to_direciton(walker_t* this, const double* probability) {
  if (*probability < this->prob.left) {
    return LEFT;
  } else if (*probability < this->prob.left + this->prob.right) {
    return RIGHT;
  } else if (* probability < this->prob.left + this->prob.right + this->prob.down) {
    return DOWN;
  } else {
    return UP;
  }
}

void walker_move(walker_t* this, position_t* newPos, const double dir_prob) {
  if (!this) {
    newPos = NULL;
    return;
  }
  movement_dir_t dir = probability_to_direciton(this, &dir_prob);
  int x = this->pos.x + DIR_DELTAS[dir].dx; 
  int y = this->pos.y + DIR_DELTAS[dir].dy;
  newPos->x = x;
  newPos->y = y;
}

//if sum of probabilities is 1 return 1, otherwise return 0
_Bool validate_probabilities(probability_dir_t* prob) {
  if (!prob) return 0;
  return prob->down + prob->up + prob->left + prob->right == 1; 
}
