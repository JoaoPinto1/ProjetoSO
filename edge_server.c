#include "edge_server.h"
pthread_mutex_t idle_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t idle_cv = PTHREAD_COND_INITIALIZER;
int idling, curr_level, pipes[2];

void edgeserver(edgeServer *server, int num, int *p)
{
    char string[40];
    snprintf(string, 39, "%s READY", server->name);
    sync_log(string, conf->log_file);

    pipes[0] = p[0];
    pipes[1] = p[1];
    idling = 0;
    begin_shm_read();
    curr_level = server->performance_lvl;
    end_shm_read();
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
    int msgid = conf->msgid;
    end_shm_read();
    float sleeptime;
    
    while (1) {
        msgrcv(msgid, &m, sizeof(msg) - sizeof(long), (long)(num + 1), 0);
        printf("%d RECEBEU %s\n", num, m.payload); 
        if (strcmp(m.payload, "MAINT") != 0) {
            strcpy(m.payload, "NO");
            msgsnd(msgid, &m, sizeof(msg) - sizeof(long), 0);
            continue;
        }
	pthread_mutex_lock(&idle_mutex);
        while (idling != 2) {
            printf("%d\n", idling);
            pthread_cond_wait(&idle_cv, &idle_mutex);
        }
        pthread_mutex_unlock(&idle_mutex);
        strcpy(m.payload, "OK");
        begin_shm_write();
	server->performance_lvl = 0;
	end_shm_write();
	printf("sending to MM: %s\n", m.payload);
	msgsnd(msgid, &m, sizeof(msg) - sizeof(long), 0);
	msgrcv(msgid, &m, sizeof(msg) - sizeof(long), (long)(num + 1), 0);
	begin_shm_write();
	server->performance_lvl = curr_level;
	end_shm_write();
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
    int v;
    idling++;
    while (1)
    {
        sem_wait(&vcpu_sems[numserver * 2 + numcpu]);
        sem_post(&vcpu_sems[numserver * 2 + numcpu]);
        printf("acordeiiiii %d-%d\n", numserver, numcpu);
        pthread_mutex_lock(&idle_mutex);
        idling--;
        pthread_cond_signal(&idle_cv);
        pthread_mutex_unlock(&idle_mutex);
       
        sem_getvalue(&vcpu_sems[numserver * 2 + numcpu], &v);
        printf("%d\n", v);
        float gosleep;
        close(pipes[1]);
        read(pipes[0], &gosleep, sizeof(float));
        printf("Server %d - VCPU %d: recebi task vou dormir: %f\n", numserver, numcpu, gosleep);
        sleep(gosleep);
        printf("Server %d - VCPU %d: dormi\n", numserver, numcpu);
        pthread_mutex_lock(&idle_mutex);
        idling++;
        pthread_cond_signal(&idle_cv);
        pthread_mutex_unlock(&idle_mutex);
        printf("%d\n", idling);
        sem_trywait(&vcpu_sems[numserver * 2 + numcpu]);
    }
    pthread_exit(NULL);
    return NULL;
}

void *workermonitor(void *ptr) 
{
    
}
