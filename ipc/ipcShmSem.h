#ifndef IPC_SHM_SEM_H
#define IPC_SHM_SEM_H
#include <game/simulation.h>

typedef struct {
    int shmid;
    int semid;
    simulation_t* p_sim;
} shm_t;

shm_t shm_init_server(int key, simulation_t* init);
shm_t shm_init_client(int key);

void shm_read(shm_t* s, char* buf, int size);
void shm_write(shm_t* s, simulation_t* src);
void shm_close(shm_t* s);

#endif

