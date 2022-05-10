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
