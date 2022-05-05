#include "task_manager.h"

queuedTask *taskQueue;
pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER, 
  scheduled_mutex = PTHREAD_MUTEX_INITIALIZER, 
  vcpu_mutex = PTHREAD_MUTEX_INITIALIZER, 
  empty_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t vcpu_cv = PTHREAD_COND_INITIALIZER,
  empty_cv = PTHREAD_COND_INITIALIZER;
void edgeserver(edgeServer server, int i);
void *workercpu();
int **pipes, pos, v;


void free_server() {

}
void taskmanager(){
    sync_log("PROCESS TASK MANAGER CREATED", conf->log_file);
    
    pipes = (int **) malloc(conf->num_servers * 2 * sizeof(int));
    for(int i = 0; i < conf->num_servers; i++){
    	pipe(pipes[i]);
    }
    queuedTask *taskQueue = (queuedTask *) malloc(sizeof(queuedTask) * conf->queuePos);
    int fd = open("TASK_PIPE", O_RDONLY);
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
    pthread_create(&threads[0], NULL, scheduler, &id[0]);
    id[1] = 1;
    pthread_create(&threads[1], NULL, dispatcher, &id[1]);

    char string[SIZETASK];
    task aux1;
    queuedTask aux2;
    r = 0;
    int to_read;
    
    while(1){
        if(read(fd, &to_read, sizeof(int)) == -1){
            sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
            exit(0);
        }
        if(read(fd, string, to_read) == -1){
            sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
            exit(0);
        }
        
        if(sscanf(string,"%[^;];%d;%d",aux1.id,&aux1.mi,&aux1.timelimit) == 3){
            pthread_mutex_lock(&task_mutex);
            aux2.t = aux1;
            aux2.priority = 0;
            aux2.arrive_time = clock();
            taskQueue[pos] = aux2;
            printf("TASK INSERIDA NA LISTA\n");
            pos++;
            pthread_mutex_lock(&vcpu_mutex);
            pthread_cond_signal(&cond_task);
            pthread_mutex_unlock(&vcpu_mutex);
        }
        
        else if(strcmp(string,"EXIT") == 0){
            sync_log("EXIT", conf->log_file);
            break;
        }
        
        else if(strcmp(string,"STATS") == 0){
            sync_log("STATS", conf->log_file);
        }
    }
    
    for(int i=0; i<2; i++){
        pthread_join(threads[i], NULL);
    }
    close(fd);
}

void *scheduler(){ //tempo de chegada + tempo maximo - tempo atual 
    while(1){
        pthread_mutex_lock(&task_mutex);
        pthread_cond_wait(&cond_task,&task_mutex);
        int len = sizeof(&taskQueue) / sizeof(queuedTask);
        for (int i = 0; i < conf->queuePos && strcmp(taskQueue[i].id, "") != 0; i++){
            taskQueue[i].priority = 1;
            for (int a = 0; a < len; i++){
                if(taskQueue[i].t.timelimit > taskQueue[a].t.timelimit){
                    taskQueue[i].priority++;
                }
                else if (taskQueue[i].t.timelimit == taskQueue[a].t.timelimit && i > a){
                    taskQueue[i].priority++;
                }
            }
        }
        for(int i = 0; i< len; i++){
        	
        }
        printf("PRIORIDADES AJUSTADAS\n");
        pthread_mutex_unlock(&task_mutex);
    }
    pthread_exit(NULL);
    return NULL;
}

void *dispatcher(){
    sem_wait(&count_cpu);
    

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
    sem_post(&count_cpu);

    pthread_exit(NULL);
    return NULL;
}
