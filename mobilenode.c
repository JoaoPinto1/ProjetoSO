#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

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

    pthread_t my_thread[numpedidos];
    int i, worker_id[numpedidos];
    for(i=0; i<numpedidos; i++){
        worker_id[i]=i;
        pthread_create(&my_thread[i], NULL, )
    }

}
