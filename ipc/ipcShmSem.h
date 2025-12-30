#ifndef IPC_SHM_SEM_H
#define IPC_SHM_SEM_H

#include <stddef.h>
#include <semaphore.h>

typedef struct {
    void* mem;
    int shmid;
    sem_t* sem;
    int size;
} shm_t;

shm_t shm_init(const char* keyFile, const char* semName, int size, int create);
void shm_write(shm_t* shm, const void* data, size_t len);
void shm_read(shm_t* shm, void* dst, size_t len);
void shm_close(shm_t* shm);

#endif

