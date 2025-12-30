#include "ipcShmSem.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

shm_t shm_init(const char* keyFile,const char* semName,int size,int create){
    shm_t m;
    key_t key = ftok(keyFile,65);
    m.size = size;
    m.shmid = shmget(key,size,0666 | (create?IPC_CREAT:0));
    m.mem = shmat(m.shmid,NULL,0);
    m.sem = sem_open(semName,O_CREAT,0644,1);
    return m;
}

void shm_write(shm_t* shm,const void* data,size_t len){
    sem_wait(shm->sem);
    memcpy(shm->mem,data,len);
    sem_post(shm->sem);
}

void shm_read(shm_t* shm,void* dst,size_t len){
    sem_wait(shm->sem);
    memcpy(dst,shm->mem,len);
    sem_post(shm->sem);
}

void shm_close(shm_t* shm){
    shmdt(shm->mem);
    sem_close(shm->sem);
}

