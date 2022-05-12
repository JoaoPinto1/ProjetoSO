/*
    log.c
    Joao Pedro Ventura Pinto 2020220907
    Tiago Oliveira Gomes 2020223013
*/

#include "log.h"
#include "shm.h"
void getcurrtime(char *curr) {
    time_t t = time(NULL);
    struct tm* info = localtime(&t);
    strftime(curr, TIMELEN, "%H:%M:%S", info);
}

void logfunc(char *s, int fd) {
    char curr[TIMELEN];
    getcurrtime(curr);
    char buffer[LOGLEN];
    int len = sprintf(buffer, "%s - %s\n", curr, s);
    printf("%s", buffer);
    write(fd, buffer, len);
}

void sync_log(char *s, int fd) {
	
    pthread_mutex_lock(&(conf->log_mutex));
    logfunc(s,fd);
    pthread_mutex_unlock(&(conf->log_mutex));
    
}
