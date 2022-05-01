/*
    log.c
    Joao Pedro Ventura Pinto 2020220907
    Tiago Oliveira Gomes 2020223013
*/

#include "log.h"

void getcurrtime(char *curr) {
    time_t t = time(NULL);
    struct tm* info = localtime(&t);
    strftime(curr, TIMELEN, "%H:%M:%S", info);
}

void logfunc(char *s, FILE *f) {
    char curr[TIMELEN];
    getcurrtime(curr);
    printf("%s - %s\n", curr, s);
    fprintf(f, "%s - %s\n", curr, s);
}

void sync_log(char *s, FILE *f) {
    pthread_mutex_lock(log_mutex);
    logfunc(s,f);
    pthread_mutex_unlock(log_mutex);
}
