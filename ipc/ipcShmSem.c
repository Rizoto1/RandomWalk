#include "ipcShmSem.h"
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static void P(int sem){ struct sembuf op={0,-1,0}; semop(sem,&op,1); }
static void V(int sem){ struct sembuf op={0, 1,0}; semop(sem,&op,1); }

shm_t shm_init_server(int key, simulation_t* init) {
    shm_t s;
    s.shmid = shmget(key,sizeof(simulation_t),IPC_CREAT|0666);
    s.p_sim = shmat(s.shmid,NULL,0);
    memcpy(s.p_sim,init,sizeof(simulation_t));
    s.semid = semget(key+1,1,IPC_CREAT|0666);
    semctl(s.semid,0,SETVAL,1);
    return s;
}

shm_t shm_init_client(int key) {
    shm_t s;
    s.shmid = shmget(key,sizeof(simulation_t),0666);
    s.p_sim = shmat(s.shmid,NULL,0);
    s.semid = semget(key+1,1,0666);
    return s;
}

void shm_read(shm_t* s, char* buf, int size) {
    P(s->semid);
    sprintf(buf,"%d/%d|%d", s->p_sim->currentReplication, s->p_sim->replications, s->p_sim->interactive);
    V(s->semid);
}

void shm_write(shm_t* s, simulation_t* src) {
    P(s->semid);
    memcpy(s->p_sim,src,sizeof(simulation_t));
    V(s->semid);
}

void shm_close(shm_t* s) {
    shmdt(s->p_sim);
}

