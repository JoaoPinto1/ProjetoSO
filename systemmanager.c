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

const char* filename = "config.txt";
int shmid;
estrutura * shared_struct;

int main(int argc, char* argv[]){
    int queuePos, maxWait, num;
    FILE * f = fopen(filename, "r");
    if (!f) {
        exit(EXIT_FAILURE);
    }
    fscanf(f,"%d %d %d", &queuePos, &maxWait, &num);
    if(num<2){
        printf("Edge server insuficientes");
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
    return 0;
}
