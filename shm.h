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
#include <sys/msg.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include "log.h"
#define SHM_NAME "SHM"
#define LOGLEN 128

typedef struct
{
    int queue_pos, max_wait, num_servers, flag_servers, msg_id, available_cpus, task_count, removed_count, log_file;
    float percent_filled, wait_time;
    pthread_mutex_t log_mutex, monitor_mutex, flag_mutex;
    pthread_cond_t monitor_cv, flag_cv;
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
    int performance_lvl, executed_count, maintenance_count;
    
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
extern pthread_mutex_t *monitor_mutex;
extern pthread_mutex_t *flag_mutex;
extern pthread_cond_t *monitor_cv;
extern pthread_cond_t *flag_cv;
void begin_shm_read();
void end_shm_read();
void begin_shm_write();
void end_shm_write();
void stats();
#endif // TASK_MANAGER_H
