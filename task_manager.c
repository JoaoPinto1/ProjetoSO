
#include "task_manager.h"

queuedTask *taskQueue;
pthread_mutex_t operation_mutex = PTHREAD_MUTEX_INITIALIZER, 
  scheduled_mutex = PTHREAD_MUTEX_INITIALIZER, 
  vcpu_mutex = PTHREAD_MUTEX_INITIALIZER, 
  empty_mutex = PTHREAD_MUTEX_INITIALIZER;
  
pthread_cond_t vcpu_cv = PTHREAD_COND_INITIALIZER,
  operation_cv = PTHREAD_COND_INITIALIZER;
  
void edgeserver(edgeServer server, int i);
void *workercpu();
int **pipes, pos, v;

int op;
void free_server() {

}
void taskmanager(){
    sync_log("PROCESS TASK MANAGER CREATED", conf->log_file);
    
    pipes = (int **) malloc(conf->num_servers * 2 * sizeof(int));
    for(int i = 0; i < conf->num_servers; i++){
    	pipe(pipes[i]);
    }
    op = 0;
	taskQueue = (queuedTask *) malloc(sizeof(queuedTask) * conf->queuePos);
    int fd = open("TASK_PIPE", O_RDWR);
    if (fd == -1){
      sync_log("ERROR OPENING TASK_PIPE", conf->log_file);
      exit(0);
    }
    /*
    for (int i = 0; i < conf->num_servers; i++){
        if (fork() == 0) {
            edgeserver(servers[i], i);
            exit(0);
        }
    }*/
    int id[2]; 
    pthread_t threads[2];
    id[0] = 0;
    pthread_create(&threads[0], NULL, scheduler, NULL);
    id[1] = 1;
    pthread_create(&threads[1], NULL, dispatcher, NULL);

    char string[SIZETASK], tid[SIZETASK], buffer[SIZETASK - 4];
    pos = 0;
    
    while(1){
    	
    	int to_read, mi, timeLimit, len;
    	strcpy(string, "");
    	strcpy(tid, "");
        if((len = read(fd, buffer, 4)) == -1){
            sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
            exit(0);
        }
        buffer[len] = '\0';
        strcat(string, buffer);
        printf("%s\n", string);
        if(strcmp(string,"EXIT") == 0){
            sync_log("EXIT", conf->log_file);
            break;
        }
        
        if((len = read(fd, buffer, 1)) == -1){
            sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
            exit(0);
        }
        buffer[len] = '\0';
        strcat(string, buffer);
        printf("%s\n", string);
        
        if(strcmp(string,"STATS") == 0){
            sync_log("STATS", conf->log_file);
            continue;
        }
        
        if((len = read(fd, buffer, SIZETASK - 4)) == -1){
            sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
            exit(0);
        }
        printf("%s\n", buffer);
        buffer[len] = '\0';
        printf("%s\n", string);
        strcat(string, buffer);
        if(sscanf(string,"%[^;];%d;%d",tid,&mi,&timeLimit) == 3){
            pthread_mutex_lock(&operation_mutex);
            while (op != 0 || pos == conf->queuePos) {
            	pthread_cond_wait(&operation_cv, &operation_mutex);
            }
            op = 0;
            pthread_cond_broadcast(&operation_cv);
    	    printf("INSERT'S TURN\n");
            strcpy(taskQueue[pos].t.id, tid);
            taskQueue[pos].priority = 0;
            taskQueue[pos].arrive_time = 20;
            printf("TASK INSERIDA NA LISTA: %s\n", taskQueue[pos].t.id);
            pos++;
            
            op = 1;
            pthread_cond_broadcast(&operation_cv);
            pthread_mutex_unlock(&operation_mutex);
        }
    }
    
    for(int i=0; i<2; i++){
        pthread_join(threads[i], NULL);
    }
    close(fd);
}

void *scheduler(){ //tempo de chegada + tempo maximo - tempo atual
    while(1){
    
    	pthread_mutex_lock(&operation_mutex);
    	while (op != 1) {
    	    pthread_cond_wait(&operation_cv, &operation_mutex);
    	}
        for (int i = 0; i < pos; i++){
            taskQueue[i].priority = 1;
            for (int a = 0; a < conf->queuePos; a++){
                if(taskQueue[i].t.timelimit > taskQueue[a].t.timelimit){
                    taskQueue[i].priority++;
                }
                else if (taskQueue[i].t.timelimit == taskQueue[a].t.timelimit && i > a){
                    taskQueue[i].priority++;
                }
            }
        }
        printf("PRIORIDADES AJUSTADAS\n");
        op = 2;
        pthread_cond_broadcast(&operation_cv);
        pthread_mutex_unlock(&operation_mutex);
    }
    pthread_exit(NULL);
    return NULL;
}

void *dispatcher(){
    while (1) {
        pthread_mutex_lock(&operation_mutex);
        while (op != 2) {
    	    pthread_cond_wait(&operation_cv, &operation_mutex);
    	}
    	printf("DISPATCHER SHIT\n");
    	op = 0;
    	pthread_cond_broadcast(&operation_cv);
        pthread_mutex_unlock(&operation_mutex);
    
    }
    pthread_exit(NULL);
    return NULL;
    
}

void edgeserver(edgeServer server, int i) {
    char string[50];
    snprintf(string,40,"%s READY",server.name);
    sync_log(string, conf->log_file);
	
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
