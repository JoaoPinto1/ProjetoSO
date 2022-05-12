#include "edge_server.h"
pthread_mutex_t idle_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t idle_cv = PTHREAD_COND_INITIALIZER;
int idling;

void edgeserver(edgeServer *server, int num)
{

    char string[40];
    snprintf(string, 39, "%s READY", server->name);
    sync_log(string, conf->log_file);

    pthread_t threads[3];
    int id[2];
    int i = 0;
    for (; i < 2; i++)
    {
        id[i] = 10 * num + i;
        pthread_create(&threads[i], NULL, workercpu, &id[i]);
    }
    
    pthread_create(&threads[i], NULL, workermonitor, &num);
    
    msg m;
    begin_shm_read();
    int id = conf->msgid;
    end_shm_read();
    float sleeptime;
    
    while (1) {
        msgrcv(id, &m, sizeof(msg) - sizeof(long), num, 0);
        if (strcmp(m.payload, "MAINT") != 0) {
            strcpy(m.payload, "NO");
            msgsnd(id, &m, sizeof(msg) - sizeof(long), 0);
            continue;
        }
        
	pthread_mutex_lock(&idle_mutex);
        while (idling != 2) {
            pthread_cond_wait(&idle_cv, &idle_mutex);
        }
        begin_shm_read();
        
        begin_shm_write();
	server->performance_level
    }
        
    for (i = 0; i < 3; i++)
    {
        pthread_join(threads[i], NULL);
    }
}

void *workercpu(void *ptr)
{
    int numserver = *(int *)ptr / 10;
    int numcpu = *(int *)ptr % 10;
    while (1)
    {
        sem_wait(&vcpu_sems[numserver * 2 + numcpu]);
        float gosleep;
        close(pipes[numserver][1]);
        read(pipes[numserver][0], &gosleep, sizeof(float));
        printf("Recebi task vou dormir: %f\n", gosleep);
        sleep(gosleep);
    }
    pthread_exit(NULL);
    return NULL;
}

