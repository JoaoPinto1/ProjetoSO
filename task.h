/*
    task.h
    Joao Pedro Ventura Pinto 2020220907
    Tiago Oliveira Gomes 2020223013
*/

#ifndef TASK_H
#define TASK_H
#include <time.h>
#include <unistd.h>
#define SIZETASK 25
#define DISPATCHEDSIZE 20
typedef struct {
    char id[SIZETASK];
    int mi;
    int timelimit;
} task;


typedef struct{
    task t;
    int priority;
    struct timespec arrive_time;
} queuedTask;

#endif //TASK_H
