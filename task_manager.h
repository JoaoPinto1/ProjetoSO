#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "shm.h"
#include "task.h"
#include "log.h"

void taskmanager();
void *scheduler();
void *dispatcher();

queuedTask *taskQueue;
               
sem_t *vcpu_sems;
void edgeserver(edgeServer server, int i);
void *workercpu();
void removeTask(int a);
int **pipes, pos, v;

enum operation
{
    insert,
    schedule,
    dispatch
} op;

#endif //TASK_MANAGER_H
