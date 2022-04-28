#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "log.h"
#include "task.h"
#define SIZETASK 30

int main(int argc, char* argv[]){
	if (argc !=5){
		printf("Argumentos em falta");
		return 1;
	}
	if(strtol(argv[1], NULL, 10)==0 || strtol(argv[2], NULL, 10) == 0 || strtol(argv[3], NULL, 10) == 0 ||
    strtol(argv[4], NULL, 10) == 0){
        printf("Argumentos inválidos");
        return 1;
    }
    long numpedidos = strtol(argv[1], NULL, 10);
    long intervalo = strtol(argv[2],NULL, 10);
    long instrucoes = strtol(argv[3],NULL,10);
    long tempomax = strtol(argv[4], NULL, 10);

    int fd = open("TASK_PIPE", O_WRONLY);
    if(fd == -1){
        printf("Named Pipe não encontrado\n");
        return 1;
    }
    char string[SIZETASK];
    for(int i = 0; i < numpedidos; i++){
        snprintf(string,SIZETASK,"%d-%d;%ld;%ld",getpid(),i,instrucoes,tempomax);
        if (write(fd, string, strlen(string) == -1)){
            printf("Erro escrever no named pipe\n");
            return 1;
        }
        sleep(intervalo);
    }
    close(fd);
}
