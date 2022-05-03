#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "shm.h"
#include "task.h"
#include "log.h"

void taskmanager();
void *scheduler();
void *dispatcher();

#endif //TASK_MANAGER_H
