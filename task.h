//
// Created by atiag on 17/04/2022.
//

#ifndef TASK_H
#define TASK_H

typedef struct {
    int mi;
    int timelimit;
} task;

typedef struct {
    task t;
    float priority;
} queuedTask;

#endif //TASK_H
