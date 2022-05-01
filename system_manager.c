/*
    system_manager.c
    Joao Pedro Ventura Pinto 2020220907
    Tiago Oliveira Gomes 2020223013
*/

#include "log.h"
#include "task.h"
#include "monitor.h"
#include "shm.h"
#define SIZETASK 30

void taskmanager();
void maintenance();
void edgeserver(edgeServer server);
void sync_log(char *s, FILE *f);
void *dispatcher();
void *scheduler();
void *workercpu();

void *shm_pointer;
configs *conf;
edgeServer *servers;
pthread_mutex_t *log_mutex;
pthread_mutex_t *task_mutex;
pthread_cond_t cond_task;
queuedTask *taskQueue;
FILE *l;

int shm_fd;
pid_t id;

int main(int argc, char *argv[]){
    if (argc != 2) {
    	printf("Not enough arguments\n");
	    exit(-1);
    }
    l = fopen(LOGFILE, "a");
    
    logfunc("OFFLOAD SIMULATOR STARTING",l);
    int queuePos, maxWait, num, offset = 0, i;
    const char* filename = argv[1];
    
	
    FILE * f = fopen(filename, "r");
    if (!f) {
        exit(EXIT_FAILURE);
    }
    fscanf(f,"%d %d %d", &queuePos, &maxWait, &num);
	
    if (num<2){
        logfunc("Edge servers insuficientes", conf->log_file);
        exit(EXIT_FAILURE);
    }
	// criacao named pipe
    if (mkfifo("TASK_PIPE", 0777) == -1){
        if(errno != EEXIST) {
            printf("Could not create named pipe\n");
            return 1;
        }
    }
    shm_fd = shm_open(SHM_NAME ,O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(configs) + sizeof(edgeServer) * num + sizeof(pthread_mutex_t));

    shm_pointer = mmap(0, sizeof(configs) + sizeof(edgeServer) * num + sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, offset);
    
    conf = (configs *) shm_pointer;
    
    conf->maxWait = maxWait;
    conf->queuePos = queuePos;
    conf->num_servers = num;
    conf->log_file = l;
    offset += sizeof(configs);
    
    servers = (edgeServer *) (shm_pointer + offset);
    for (i = 0; i < num; i++) {
        fscanf(f," %[^,],%d,%d", servers[i].name, &(servers[i].vcpus[0].speed), &(servers[i].vcpus[1].speed));
    }
	
    fclose(f);
    
    offset += num * sizeof(edgeServer);
    log_mutex = (pthread_mutex_t *) (shm_pointer + offset);
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(log_mutex, &attr);
    sync_log("SHARED MEMORY CREATED", conf->log_file);
	
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
    pthread_mutex_destroy(log_mutex);
    pthread_mutex_destroy(task_mutex);
    pthread_cond_destroy(&cond_task);
    sync_log("SIMULATOR CLOSING", conf->log_file);
    return 0;
}

void maintenance() {
    sync_log("PROCESS MAINTENANCE MANAGER CREATED", conf->log_file);
}

void taskmanager(){
    sync_log("PROCESS TASK MANAGER CREATED", conf->log_file);
    taskQueue = (queuedTask *) malloc(sizeof(queuedTask) * conf->queuePos);
    int fd = open("TASK_PIPE", O_RDONLY);
    if (fd == -1){
        sync_log("ERROR OPENING TASK_PIPE", conf->log_file);
        exit(0);
    }
    for (int i = 0; i < conf->num_servers; i++){
        if ((id = fork()) == 0) {
            edgeserver(servers[i]);
            exit(0);
        }
    }
    char string[SIZETASK];
    task aux1;
    queuedTask aux2;
    
    pthread_t threads[2];
    int id[2];


    int offset = 0;
    while(1){
        if(read(fd, string, SIZETASK) == -1){
            sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
            exit(0);
        }
        if(sscanf(string,"%s;%d;%d",aux1.id,&aux1.mi,&aux1.timelimit) == 3){
            pthread_mutex_lock(task_mutex);

            if(offset == 0){
                id[0] = 0;
                pthread_create(&threads[0], NULL, scheduler, &id[0]);
                id[1] = 1;
                pthread_create(&threads[1], NULL, dispatcher, &id[1]);
            }

            aux2.t = aux1;
            aux2.priority = 0;
            aux2.arrive_time = time(NULL);
            taskQueue[offset] = aux2;
            printf("TASK INSERIDA NA LISTA\n");
            offset++;
            pthread_mutex_unlock(task_mutex);
            pthread_cond_signal(&cond_task);
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

void edgeserver(edgeServer server) {
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


void *scheduler(){ //tempo de chegada + tempo maximo - tempo atual 
    while(1){
        pthread_mutex_lock(task_mutex);
        pthread_cond_wait(&cond_task,task_mutex);
        int len = sizeof(&taskQueue) / sizeof(queuedTask);
        for (int i = 0; i < len; i++){
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
        printf("PRIORIDADES AJUSTADAS\n");
        pthread_mutex_unlock(task_mutex);
    }
    pthread_exit(NULL);
    return NULL;
}

void *dispatcher(){
    pthread_exit(NULL);
    return NULL;
}


