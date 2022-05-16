#include "shm.h"

void begin_shm_read() {
    pthread_mutex_lock(&(rdwr_lock->read_mutex));
    rdwr_lock->b++;
    if (rdwr_lock->b == 1) pthread_mutex_lock(&(rdwr_lock->global_mutex));
    pthread_mutex_unlock(&(rdwr_lock->read_mutex));
}

void end_shm_read() {
    pthread_mutex_lock(&(rdwr_lock->read_mutex));
    rdwr_lock->b--;
    if (rdwr_lock->b == 0) pthread_mutex_unlock(&(rdwr_lock->global_mutex));
    pthread_mutex_unlock(&(rdwr_lock->read_mutex));
}

void begin_shm_write() {
    pthread_mutex_lock(&(rdwr_lock->global_mutex));
}

void end_shm_write() {
    pthread_mutex_unlock(&(rdwr_lock->global_mutex));
}


void stats() {
    begin_shm_read();
    printf("\nSTATS:\n");
    printf("Number of tasks executed: %d\n", conf->task_count);
    if(conf->task_count == 0)
    	printf("Average time of execution: 0\n");
    else
    	printf("Average time of execution: %f\n", conf->wait_time/(float)conf->task_count);
    for (int i = 0; i < conf->num_servers; i++) {
        printf(">%s: %d executed tasks, %d maintenence operations\n", servers[i].name, servers[i].executed_count, servers[i].maintenance_count);
    }
    printf("Number of tasks removed: %d\n", conf->removed_count);
    end_shm_read();
}
