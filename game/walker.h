#ifndef WALKER_H
#define WALKER_H

//current Walker position
typedef struct {
  int x;
  int y;
} position_t;

//Walker directional probabilites
typedef struct {
  double up;
  double down;
  double left;
  double right;
} probability_dir_t;

typedef enum {
  UP,
  DOWN,
  RIGHT,
  LEFT
} movement_dir_t;

typedef struct {
  position_t pos;
  probability_dir_t prob;
} walker_t;

void walker_init(walker_t* this, int* x, int* y, double* probUp, double* probDown, double* probRight, double* probLeft);
void walker_destroy(walker_t* this);
void move_up();
void move_down();
void move_left();
void move_right();



#endif //WALKER_H
