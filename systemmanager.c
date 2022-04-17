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

void taskmanager(int num, edgeServer* servers);
void monitor();
void maintenance();
void edgeserver(edgeServer server);
void sync_log(char *s);

configs *conf;
edgeServer *servers;
pthread_mutex_t *log_mutex;

const char* filename = "config.txt";
int shm_fd;
pid_t id;

int main(){
    int queuePos, maxWait, num, offset = 0;
    FILE * f = fopen(filename, "r");

    if (!f) {
        exit(EXIT_FAILURE);
    }
    sync_log("OFFLOAD SIMULATOR STARTING");

    fscanf(f,"%d %d %d", &queuePos, &maxWait, &num);
    if(num<2){
        sync_log("Edge servers insuficientes");
        exit(EXIT_FAILURE);
    }

    int i;

    shm_fd = shm_open(SHM_NAME ,O_CREAT | O_RDWR, 0666);

    ftruncate(shm_fd, sizeof(configs) + sizeof(edgeServer) * num + sizeof(pthread_mutex_t));

    conf = (configs *) mmap(0, sizeof(configs), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, offset);

    conf->maxWait = maxWait;
    conf->queuePos = queuePos;
    conf->num_servers = num;

    offset += sizeof(configs);

    servers = (edgeServer *) mmap(0, sizeof(edgeServer) * num, PROT_READ | PROT_WRITE,  MAP_SHARED, shm_fd, offset);


    for(i=0; i < num; i++){
        fscanf(f,"%[^,],%d,%d ", servers[i].name, &servers[i].vcpus[0].speed, &servers[i].vcpus[1].speed);
    }
	
	fclose(f);
    offset += num * sizeof(edgeServer);

    log_mutex = (pthread_mutex_t *) mmap(0, sizeof(edgeServer) * num, PROT_READ | PROT_WRITE,  MAP_SHARED, shm_fd, offset);
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(log_mutex, &attr);

    sync_log("SHARED MEMORY CREATED");
	
    int id1 = fork();
    int id2 = fork();

    if(id1==0){

        if(id2 == 0) {
            taskmanager(num, servers);

        } else {
            monitor();
        }

    } else {

        if(id2 == 0){
            maintenance();
        }
    }


    shm_unlink(SHM_NAME);
    return 0;
}

void maintenance() {
    printf("%d %d %d\n", conf->queuePos, conf->maxWait, conf->num_servers);
    for (int i = 0; i < conf->num_servers; i++) {
        printf("%s %d %d\n", servers[i].name, servers[i].vcpus[0].speed, servers[i].vcpus[1].speed);
    }
}

void taskmanager(int num, edgeServer* servers){
    int i;

    for(i = 0;i < num; i++){
        if((id=fork())==0) {
            edgeserver(servers[i]);
        }
    }
}

void edgeserver(edgeServer server) {

}

void monitor() {

}
void sync_log(char *s) {
    pthread_mutex_lock(log_mutex);
    logfunc(s);
    pthread_mutex_unlock(log_mutex);
}
