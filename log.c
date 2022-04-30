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
    printf("OLA");
    fprintf(f, "%s - %s\n", curr, s);
    
    fclose(f);
}
