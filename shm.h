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

typedef struct
{
    int queue_pos, max_wait, num_servers, flag_servers, log_file;
    float percent_filled, wait_time;
    pthread_mutex_t log_mutex;
} configs;

typedef struct
{
    int speed;
    float wait_time;
} vcpu;

typedef struct
{
    char name[32];
    vcpu vcpus[2];
    int performance_lvl;
} edgeServer;

typedef struct
{
    pthread_mutex_t read_mutex;
    pthread_mutex_t global_mutex;
    int b;
} readwrite_lock;

typedef struct {
    configs c;
    readwrite_lock l;
} shm;
    
extern void *shm_pointer;
extern configs *conf;
extern edgeServer *servers;
extern readwrite_lock *rdwr_lock;

void begin_shm_read();
void end_shm_read();
void begin_shm_write();
void end_shm_write();

#endif // TASK_MANAGER_H
