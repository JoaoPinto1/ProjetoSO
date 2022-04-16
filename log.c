#include "log.h"

void getcurrtime(char *curr) {
    time_t t = time(NULL);
    struct tm info = localtime(&t);
    strftime(curr, TIMELEN, "%H:%M:%S", &info);
}

void logfunc(char *s) {
    char curr[TIMELEN];
    getcurrtime(curr);

    FILE *f = fopen(LOGFILE, "a");
    printf("%s - %s\n", curr, s);
    fprintf(f, "%s - %s\n", curr, s);
}