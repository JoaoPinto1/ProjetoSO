/*
    log.h
    Joao Pedro Ventura Pinto 2020220907
    Tiago Oliveira Gomes 2020223013
*/

#ifndef LOG_H
#define LOG_H

#define LOGFILE "log.txt"
#define TIMELEN 9
#include <stdio.h>
#include <time.h>

void getcurrtime(char *);
void logfunc(char *);

#endif //LOG_H
