#ifndef SHM_H
#define SHM_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#define SHM_NAME "SHM"

typedef struct {
    int queuePos, maxWait, num_servers, perf_flag;
    FILE *log_file;
} configs;

typedef struct {
    int speed;
    float wait_time;
    int nivel;
} vcpu;

typedef struct{
    char name[32];
    vcpu vcpus[2];
} edgeServer;

extern void *shm_pointer;
extern configs *conf;
extern edgeServer *servers;
extern pthread_mutex_t *log_mutex;


#endif //TASK_MANAGER_H
