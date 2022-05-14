#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "shm.h"
#include "task.h"
#include "log.h"
#include "edge_server.h"
#include "task_shm.h"
void taskmanager();
void *scheduler();
void *dispatcher();
          
void removeTask(int a);

enum operation
{
    insert,
    schedule,
    dispatch
} op;

queuedTask *taskQueue;

int timespec_cmp(struct timespec t1, struct timespec t2);
void timespec_subtraction(struct timespec t1, struct timespec t2, struct timespec *t3);
void double_to_timespec(double d, struct timespec *t);
double timespec_to_double(struct timespec t);

#endif //TASK_MANAGER_H
