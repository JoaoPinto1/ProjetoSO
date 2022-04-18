/*
    system_manger.c
    Joao Pedro Ventura Pinto 2020220907
    Tiago Oliveira Gomes 2020223013
*/

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
#define SHM_NAME "SHM"

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

typedef struct{
    task t;
    float priority;
} queuedTask;

void taskmanager();
void monitor();
void maintenance();
void edgeserver(edgeServer server);
void sync_log(char *s);
void *dispatcher();
void *scheduler();
void *workercpu();

void *shm_pointer;
configs *conf;
edgeServer *servers;
pthread_mutex_t *log_mutex;

int shm_fd;
pid_t id;

int main(int argc, char *argv[]){
    if (argc != 2) {
	    logfunc("INCORRECT NUMBER OF ARGUMENTS");
	    exit(-1);
    }
	
    logfunc("OFFLOAD SIMULATOR STARTING");

    int queuePos, maxWait, num, offset = 0, i;
    const char* filename = argv[1];
	
    FILE * f = fopen(filename, "r");
    if (!f) {
	logfunc("ERROR READING CONFIGURATION FILE");
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
	
    servers = (edgeServer *) (shm_pointer + offset);
	
    for (i = 0; i < num; i++){
        fscanf(f," %[^,],%d,%d", servers[i].name, &(servers[i].vcpus[0].speed), &(servers[i].vcpus[1].speed));
    }
	
    fclose(f);
    offset += num * sizeof(edgeServer);
    log_mutex = (pthread_mutex_t *) (shm_pointer + offset);
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(log_mutex, &attr);

    sync_log("SHARED MEMORY CREATED");
    
    for (i = 0; i < 3; i++) {
    	if (fork()==0) {
    		switch(i) {
    		
    		case 0:
    			taskmanager();
    			exit(0);
    			break;
    			
    		case 1:
    			maintenance();
    			exit(0);
    			break;
    			
    		default:
    			monitor();
    			exit(0);
    			break;
    			
    		}
    	}	
    }

    for (i=0; i < 3; i++){
        wait(NULL);
    }
    
    shm_unlink(SHM_NAME);
    sync_log("SIMULATOR CLOSING");
    return 0;
}

void maintenance() {
    sync_log("PROCESS MAINTENANCE MANAGER CREATED");
}

void taskmanager(){
    sync_log("PROCESS TASK MANAGER CREATED");
    //queuedTask *taskQueue = (queuedTask *) malloc(sizeof(queuedTask) * conf->queuePos);
    int i;

    for (i = 0; i < conf->num_servers; i++){
        if ((id = fork()) == 0) {
            edgeserver(servers[i]);
            exit(0);
        }
    }
    
    for (i=0; i < conf->num_servers; i++) {
        wait(NULL);
    }
    pthread_t threads[2];
    int id[2];
    id[0] = 0;
    pthread_create(&threads[0], NULL, scheduler, &id[0]);
    id[1] = 1;
    pthread_create(&threads[1], NULL, dispatcher, &id[1]);
	
    for(i=0; i<2; i++){
        pthread_join(threads[i], NULL);
    }
}

void edgeserver(edgeServer server) {
    char string[50];
    snprintf(string,40,"%s READY",server.name);
    sync_log(string);
	
    pthread_t threads[2];
    int id[2];
	
    for (int i=0; i<2; i++){
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
    sync_log("PROCESS MONITOR CREATED");
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
