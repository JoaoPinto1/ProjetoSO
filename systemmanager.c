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
int shmid;
estrutura * shared_struct;
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
    edgeServer servers[num];
	
    for(i=0; i < num; i++){
        fscanf(f,"%[^,],%d,%d ", servers[i].nome, &servers[i].vcpu1, &servers[i].vcpu2);
    }

    if((shmid = shmget(IPC_PRIVATE,sizeof(estrutura),IPC_CREAT | 0766))<0){
        perror("Error in shmget");
        exit(1);
    }
	
    if((shared_struct = (estrutura*) shmat(shmid,NULL,0))==(estrutura*)-1){
	perror("Shmat error");
	exit(1);
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
	    char s[15];
	    sprintf(s, "SERVER_%d READY", i+1);
	    logfunc(s);
	    
            edgeserver(servers[i]);
	}
    }
}

void edgeserver(edgeServer server) {
	
}
