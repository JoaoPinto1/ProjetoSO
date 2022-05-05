/*
    system_manager.c
    Joao Pedro Ventura Pinto 2020220907
    Tiago Oliveira Gomes 2020223013
*/

#include "log.h"
#include "task.h"
#include "monitor.h"
#include "task_manager.h"
#include "shm.h"

//void taskmanager();
void maintenance();
//void edgeserver(edgeServer server);
void sync_log(char *s, FILE *f);
//void *dispatcher();
//void *scheduler();
//void *workercpu();

void *shm_pointer;
configs *conf;
edgeServer *servers;
pthread_mutex_t *log_mutex;

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
    
	
    FILE * f = fopen(filename, "r");printf("boas\n");
    if (!f) {
        exit(EXIT_FAILURE);
    }
    fscanf(f,"%d %d %d", &queuePos, &maxWait, &num);
	
    if (num<2){
        logfunc("Edge servers insuficientes", conf->log_file);
        exit(EXIT_FAILURE);
    }
	// criacao named pipe
    if (mkfifo("TASK_PIPE", O_CREAT|O_EXCL|0777) == -1){
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
    conf->perf_flag = 1;
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
    sync_log("SIMULATOR CLOSING", conf->log_file);
    return 0;
}

void maintenance() {
    sync_log("PROCESS MAINTENANCE MANAGER CREATED", conf->log_file);
}
