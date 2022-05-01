/*
    log.h
    Joao Pedro Ventura Pinto 2020220907
    Tiago Oliveira Gomes 2020223013
*/

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <time.h>
#include "shm.h"
#define LOGFILE "log.txt"
#define TIMELEN 9

void getcurrtime(char *);
void logfunc(char *,FILE *);
void sync_log(char *s, FILE *f);

#endif //LOG_H
