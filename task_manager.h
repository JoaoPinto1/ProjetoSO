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

queuedTask *taskQueue;
          
void removeTask(int a);

enum operation
{
    insert,
    schedule,
    dispatch
} op;

#endif //TASK_MANAGER_H
