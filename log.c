/*
    log.c
    Joao Pedro Ventura Pinto 2020220907
    Tiago Oliveira Gomes 2020223013
*/

#include "log.h"

void getcurrtime(char *curr) {
    time_t t = time(NULL);
    struct tm* info = localtime(&t);
    strftime(curr, TIMELEN, "%d/%m/%Y - %H:%M:%S", info);
}

void logfunc(char *s) {
    char curr[TIMELEN];
    getcurrtime(curr);

    FILE *f = fopen(LOGFILE, "a");
    printf("%s - %s\n", curr, s);
    fprintf(f, "%s - %s\n", curr, s);
    
    fclose(f);
}
