/*
    task.h
    Joao Pedro Ventura Pinto 2020220907
    Tiago Oliveira Gomes 2020223013
*/

#ifndef TASK_H
#define TASK_H

typedef struct {
    char *id;
    int mi;
    int timelimit;
} task;


typedef struct{
    task t;
    float arrive_time;
    int priority;
} queuedTask;

#endif //TASK_H