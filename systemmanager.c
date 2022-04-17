#define SHM_NAME "SHM"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "log.h"
#include "task.h"

typedef struct{
    int queuePos;
    int maxWait;
    int num_servers;
} configs;

typedef struct{
    int speed;
    float wait_time;
    int nivel;
} vcpu;

typedef struct{
    char name[32];
    vcpu vcpus[2];
} edgeServer;

void taskmanager();
void monitor();
void maintenance();
void edgeserver(edgeServer server, int num);
void sync_log(char *s);

void *shm_pointer;
configs *conf;
edgeServer *servers;
pthread_mutex_t *log_mutex;
task *taskQueue;

const char* filename = "config.txt";
int shm_fd;
pid_t id;

int main(){
    int queuePos, maxWait, num, offset = 0, i;
    logfunc("OFFLOAD SIMULATOR STARTING");
	
    FILE * f = fopen(filename, "r");
    if (!f) {
        exit(EXIT_FAILURE);
    }
    fscanf(f,"%d %d %d", &queuePos, &maxWait, &num);
	
    if (num<2){
        sync_log("Edge servers insuficientes");
        exit(EXIT_FAILURE);
    }
	
    shm_fd = shm_open(SHM_NAME ,O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(configs) + sizeof(edgeServer) * num + sizeof(pthread_mutex_t));

    shm_pointer = mmap(0, sizeof(configs) + sizeof(edgeServer) * num + sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, offset);
    
    conf = (configs *) shm_pointer;
    
    conf->maxWait = maxWait;
    conf->queuePos = queuePos;
    conf->num_servers = num;
    offset += sizeof(configs);

    taskQueue = (task *) malloc(sizeof(task) * queuePos);
	
    servers = (edgeServer *) (shm_pointer + offset);
	
    for (i = 0; i < num; i++){
        fscanf(f,"%[^,],%d,%d", servers[i].name, &(servers[i].vcpus[0].speed), &(servers[i].vcpus[1].speed));
    }
	
    fclose(f);
    offset += num * sizeof(edgeServer);
    log_mutex = (pthread_mutex_t *) (shm_pointer + offset);
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(log_mutex, &attr);

    sync_log("SHARED MEMORY CREATED");
	
    int id1 = fork();
    int id2 = fork();

    if (id1 == 0){

        if (id2 == 0) {
            taskmanager();

        } else {
            monitor();
        }

    } else {

        if (id2 == 0){
            maintenance();
        }
    }

    for (i=0; i < 3; i++){
        wait(NULL);
    }
    shm_unlink(SHM_NAME);
    return 0;
}

void maintenance() {

}

void taskmanager(){
    logfunc("PROCESS TASK MANAGER CREATED");
    int i;

    for (i = 0; i < conf->num_servers; i++){
        if ((id = fork()) == 0) {
            edgeserver(servers[i], i);
            exit(0);
        }
    }
    pthread_t threads[2];
    int id[2];
    for(int i=0, i<2; i++){
        id[i] = i;
        pthread_create(&threads[i], NULL, workercpu, &id[i]);
    }
	
    for(int i=0;i<2;i++){
        pthread_join(threads[i], NULL);
    }
}

void edgeserver(edgeServer server, int num) {
    char string[50];
    snprintf(string,20,"SERVER_%d READY",num);
    logfunc(string);
	
    pthread_t threads[2];
    int id[2];
	
    for (int i=0, i<2; i++){
        id[i] = i;
        pthread_create(&threads[i],NULL, workercpu, &id[i]);
    }

    for (int i=0; i<2; i++){
        pthread_join(threads[i],NULL);
    }
}

void *workercpu(){
    pthread_exit(NULL);
    return NULL;
}

void monitor() {

}

void *scheduler(){

    pthread_exit(NULL);
    return NULL;
}

void *dispatcher(){

    pthread_exit(NULL);
    return NULL;
}
void sync_log(char *s) {
    pthread_mutex_lock(log_mutex);
    logfunc(s);
    pthread_mutex_unlock(log_mutex);
}
