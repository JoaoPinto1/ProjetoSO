#include "edge_server.h"
pthread_mutex_t idle_mutex = PTHREAD_MUTEX_INITIALIZER, maint_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t idle_cv = PTHREAD_COND_INITIALIZER, maint_cv = PTHREAD_COND_INITIALIZER;
int n;
int idling, curr_level, pipes[2];
pthread_t threads[3];
void sigint_edgeServer();

void edgeserver(edgeServer *server, int num, int *p)
{
	sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGUSR1);
    sigprocmask(SIG_BLOCK,&set,NULL);
	signal(SIGUSR1,sigint_edgeServer);
    char string[40];
    snprintf(string, 39, "%s READY", server->name);
    sync_log(string, conf->log_file);
    n = num;
    pipes[0] = p[0];
    pipes[1] = p[1];
    idling = 0;
    begin_shm_read();
    curr_level = server->performance_lvl;
    end_shm_read();
    
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
    int msgid = conf->msg_id;
    end_shm_read();
    
    while (1) {
        msgrcv(msgid, &m, sizeof(msg) - sizeof(long), (long)(num + 1), 0);
       
        if (strcmp(m.payload, "MAINT") != 0) {
            strcpy(m.payload, "NO");
            msgsnd(msgid, &m, sizeof(msg) - sizeof(long), 0);
            continue;
        }
	    pthread_mutex_lock(&idle_mutex);
	    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        while (idling != 2) {
            pthread_cond_wait(&idle_cv, &idle_mutex);
        }
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_mutex_unlock(&idle_mutex);
        strcpy(m.payload, "OK");
        
        pthread_mutex_lock(&(tm_shm->mutex));
        
        begin_shm_write();
		server->performance_lvl = 0;
		conf->available_cpus -= curr_level;
		end_shm_write();
	
		pthread_cond_broadcast(&(tm_shm->cv));
		pthread_mutex_unlock(&(tm_shm->mutex));
	
		msgsnd(msgid, &m, sizeof(msg) - sizeof(long), 0);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		msgrcv(msgid, &m, sizeof(msg) - sizeof(long), (long)(num + 1), 0);
	
		pthread_mutex_lock(&(tm_shm->mutex));
		begin_shm_write();
		server->performance_lvl = curr_level;
		conf->available_cpus += curr_level;
		servers[num].maintenance_count++;
		end_shm_write();
		pthread_cond_broadcast(&(tm_shm->cv));
		pthread_mutex_unlock(&(tm_shm->mutex));
		pthread_mutex_lock(&maint_mutex);
		pthread_cond_signal(&maint_cv);
		pthread_mutex_unlock(&maint_mutex);
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
    int len;
    char received[DISPATCHEDSIZE], id[DISPATCHEDSIZE] = "", gosleep[DISPATCHEDSIZE] = "";
    idling++;
    while (1)
    {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        sem_wait(&vcpu_sems[numserver * 2 + numcpu]);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        sem_post(&vcpu_sems[numserver * 2 + numcpu]);
       
        pthread_mutex_lock(&idle_mutex);
        idling--;
        pthread_cond_signal(&idle_cv);
        pthread_mutex_unlock(&idle_mutex);
       

        close(pipes[1]);
        if ((len = read(pipes[0], received, DISPATCHEDSIZE)) == -1)
        {
            sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
            exit(0);
        }
        received[len] = '\0';
        if (sscanf(received, "%[^;];%[^;]", id, gosleep) == 2) 
        {
            double sleeptime = strtod(gosleep, NULL);
            printf("%s - VCPU %d: recebi task %s vou dormir: %f\n", servers[numserver].name, numcpu, id, sleeptime);
            sleep(sleeptime);
            printf("%s - VCPU %d: done!\n", servers[numserver].name, numcpu);
        }
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_mutex_lock(&idle_mutex);
        idling++;
        pthread_cond_signal(&idle_cv);
        pthread_mutex_unlock(&idle_mutex);
        pthread_mutex_lock(&(tm_shm->mutex));
        begin_shm_write();
        conf->available_cpus++;
        end_shm_write();
        pthread_cond_signal(&(tm_shm->cv));
        pthread_mutex_unlock(&(tm_shm->mutex));
        sem_trywait(&vcpu_sems[numserver * 2 + numcpu]);
    }
    pthread_exit(NULL);
    return NULL;
}

void *workermonitor(void *ptr) 
{
	int numserver = *(int *)ptr;
	int flag, curr_flag, performance_level;
    while(1){
    	begin_shm_read();
    	curr_flag = conf->flag_servers;
    	flag = conf->flag_servers;
    	end_shm_read();
    	pthread_mutex_lock(flag_mutex);
    	while(flag == curr_flag){
    	    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    		pthread_cond_wait(flag_cv,flag_mutex);
    		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    		begin_shm_read();
    		performance_level = servers[numserver].performance_lvl;
    		flag = conf->flag_servers;
    		end_shm_read();
    	}
    	pthread_mutex_unlock(flag_mutex);
    	pthread_mutex_lock(&maint_mutex);
    	while (performance_level == 0){
    	    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    		pthread_cond_wait(&maint_cv, &maint_mutex);
    		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    	}
    	pthread_mutex_unlock(&maint_mutex);
    	begin_shm_write();
    	pthread_mutex_lock(&(tm_shm->mutex));
    	servers[numserver].performance_lvl = flag;
    	conf->available_cpus += (flag-curr_flag);
    	pthread_cond_signal(&(tm_shm->cv));
        pthread_mutex_unlock(&(tm_shm->mutex));
    	end_shm_write();
    }
}

void sigint_edgeServer(){
servers[n].performance_lvl =0;
	pthread_mutex_lock(&idle_mutex);
	
	while (idling != 2) {
		pthread_cond_wait(&idle_cv, &idle_mutex);
		
    }
    pthread_mutex_unlock(&idle_mutex);
	pthread_cond_broadcast(&maint_cv);
	pthread_cond_broadcast(&(tm_shm->cv));
	
	for(int i = 0; i<3 ; i++){
		pthread_cancel(threads[i]);
	}
	idle_cv.__data.__wrefs = 0;
	maint_cv.__data.__wrefs = 0;
	pthread_mutex_destroy(&idle_mutex);
	pthread_cond_destroy(&idle_cv);
	pthread_mutex_destroy(&maint_mutex);
	pthread_cond_destroy(&maint_cv);
	exit(0);
}
