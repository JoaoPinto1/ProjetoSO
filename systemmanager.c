#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "log.h"
#DEFINE SHM_NAME "SHM"
typedef struct{
    char nome[32];
    int vcpu1;
    int vcpu2;
} edgeServer;

typedef struct{
	int capacidade[2];
	float time[2];
	int nivel[2];
} estrutura;

void taskmanager(int num, edgeServer* servers);
void monitor();
void maintenance();
void edgeserver(edgeServer server);



const char* filename = "config.txt";
int shm_fd;
estrutura * shared_struct;
edgeServer *servers;
pid_t id;

int main(int argc, char* argv[]){
    int queuePos, maxWait, num;
    FILE * f = fopen(filename, "r");
	
    if (!f) {
        exit(EXIT_FAILURE);
    }
	
    fscanf(f,"%d %d %d", &queuePos, &maxWait, &num);
    if(num<2){
        logfunc("Edge servers insuficientes");
        exit(EXIT_FAILURE);
    }
	
    int i;
	
    if((shm_fd = shm_open(SHM_NAME ,O_CREAT | O_RDWR, 0666))<0){
        perror("Error in shm_open");
        exit(1);
    }
	
    ftruncate(shmid, sizeof(estrutura) + sizeof(edgeServer) * num);
	
    if((shared_struct = (estrutura *) mmap(0, sizeof(estrutura), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0) < 0){
	perror("mmap error - shared_struct");
	exit(1);
    }
       
    if((servers = (edgeServer *) mmap(0, sizeof(edgeServer) * num, PROT_READ | PROT_WRITE,  MAP_SHARED, shm_fd, sizeof(estrutura)) < 0){
	perror("mmap error - servers");
	exit(1);
    }
    for(i=0; i < num; i++){
        fscanf(f,"%[^,],%d,%d ", servers[i].nome, &servers[i].vcpu1, &servers[i].vcpu2);
    }
	
    
    logfunc("SHARED MEMORY CREATED");
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

    return 0;
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
    char s[15];
    sprintf(s, "SERVER_%d READY", i+1);
    logfunc(s);
}
