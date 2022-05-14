#ifndef TASK_SHM_H
#define TASK_SHM_H
#include <pthread.h>
#include <semaphore.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cv;
} taskmanager_shm;
extern sem_t *vcpu_sems;
extern taskmanager_shm * tm_shm;

#endif //TASK_MANAGER_H
