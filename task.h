//
// Created by atiag on 17/04/2022.
//

#ifndef EX_B_C_TASK_H
#define EX_B_C_TASK_H

typedef struct {
    int mi;
    int timelimit;
} task;

typedef struct {
    task t;
    float priority;
} queuedTask;

#endif //EX_B_C_TASK_H
