#include "task_manager.h"
pthread_mutex_t operation_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t operation_cv = PTHREAD_COND_INITIALIZER;

char logstring[128];
sem_t *vcpu_sems;
int **pipes, pos, v;
int task_fd;
taskmanager_shm *tm_shm;
int num_servers, queue_pos;
bool waiting;
pid_t *ids;
pthread_t threads[2];
int fd;

void int_timespec_subtraction(int sec, struct timespec t1, struct timespec *t2);
void taskmanager()
{
    sync_log("PROCESS TASK MANAGER CREATED", conf->log_file);
	signal(SIGUSR1,sigint_task);
    begin_shm_read();
    num_servers = conf->num_servers;
    queue_pos = conf->queue_pos;
    end_shm_read();

    taskQueue = (queuedTask *)malloc(queue_pos * sizeof(queuedTask));

    task_fd = shm_open(TASK_SHMNAME, O_CREAT | O_RDWR, 0666);
    ftruncate(task_fd, sizeof(taskmanager_shm) + num_servers * 2 * sizeof(sem_t));
    void *shm_pointer = mmap(0, sizeof(taskmanager_shm) + num_servers * 2 * sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, task_fd, 0);
    tm_shm = (taskmanager_shm *)shm_pointer;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    pthread_condattr_t attr_cv;
    pthread_condattr_init(&attr_cv);
    pthread_condattr_setpshared(&attr_cv, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&(tm_shm->mutex), &attr);
    pthread_cond_init(&(tm_shm->cv), &attr_cv);

    vcpu_sems = (sem_t *)(shm_pointer + sizeof(taskmanager_shm));

    for (int i = 0; i < num_servers * 2; i++)
    {
        sem_init(&(vcpu_sems[i]), 1, 0);
    }

    pipes = (int **)malloc(num_servers * sizeof(int *));

    for (int i = 0; i < num_servers; i++)
    {
        pipes[i] = (int *)malloc(2 * sizeof(int));
        pipe(pipes[i]);
    }

    fd = open("TASK_PIPE", O_RDWR);
    if (fd == -1)
    {
        sync_log("ERROR OPENING TASK_PIPE", conf->log_file);
        exit(0);
    }
	
	ids = (pid_t *) malloc(num_servers * sizeof(pid_t));
    for (int i = 0; i < num_servers; i++)
    {
        if ((ids[i] = fork()) == 0)
        {
            edgeserver(&servers[i], i, pipes[i]);
            exit(0);
        }
    }

    int id[2];
    id[0] = 0;
    pthread_create(&threads[0], NULL, scheduler, &id[0]);
    id[1] = 1;
    pthread_create(&threads[1], NULL, dispatcher, &id[1]);

    char string[SIZETASK], tid[SIZETASK], buffer[SIZETASK - 4];
    pos = 0;
    op = insert;
    waiting = false;

    while (1)
    {
        int mi, len, timeLimit;
        strcpy(string, "");
        strcpy(tid, "");
        if ((len = read(fd, buffer, 4)) == -1)
        {
            sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
            exit(0);
        }
        buffer[len] = '\0';
        strcat(string, buffer);
        if (strcmp(string, "EXIT") == 0)
        {
        	read(fd, buffer, 1);
            sync_log("EXIT", conf->log_file);
            kill(getppid(),SIGINT);
            
        }
        if ((len = read(fd, buffer, 1)) == -1)
        {
            sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
            exit(0);
        }
        buffer[len] = '\0';
        strcat(string, buffer);
        if (strcmp(string, "STATS") == 0)
        {
        	read(fd, buffer, 1);
            stats();
            continue;
        }

        if ((len = read(fd, buffer, SIZETASK - 5)) == -1)
        {
            sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
            exit(0);
        }

        buffer[len] = '\0';
        strcat(string, buffer);
        if (sscanf(string, "%[^;];%d;%d", tid, &mi, &timeLimit) == 3)
        {
            pthread_mutex_lock(&operation_mutex);

            while (op != insert)
            {
            	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
                pthread_cond_wait(&operation_cv, &operation_mutex);
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            }
            if (pos == queue_pos){
            	sync_log("TASK REMOVED DUE TO LACK OF SPACE IN QUEUE",conf->log_file);
            	conf->removed_count++;
            	op = dispatch;
            	pthread_cond_broadcast(&operation_cv);
            	pthread_mutex_unlock(&operation_mutex);
            	continue;
            }
            strcpy(taskQueue[pos].t.id, tid);
            taskQueue[pos].t.mi = mi;
            taskQueue[pos].t.timelimit = timeLimit;
            taskQueue[pos].priority = 0;
            clock_gettime(CLOCK_REALTIME, &(taskQueue[pos].arrive_time));
            pthread_mutex_lock(monitor_mutex);
            begin_shm_write();
            conf->percent_filled += (float)1/(float)queue_pos;
            end_shm_write();
			pthread_mutex_unlock(monitor_mutex);
            pthread_cond_signal(monitor_cv);
            snprintf(logstring, LOGLEN, "TASK INSERIDA NA FILA: %s«\n", taskQueue[pos].t.id);
            sync_log(logstring, conf->log_file);
            pos++;
            op = schedule;
            pthread_cond_broadcast(&operation_cv);
            pthread_mutex_unlock(&operation_mutex);
            continue;
        }
    }

    for (int i = 0; i < 2; i++)
    {
        pthread_join(threads[i], NULL);
    }
    close(fd);
}

void *scheduler()
{ // tempo de chegada + tempo maximo - tempo atual
    while (1)
    {
        pthread_mutex_lock(&operation_mutex);
        while (op != schedule)
        {
        	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            pthread_cond_wait(&operation_cv, &operation_mutex);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        }
        struct timespec current_time, elapsed_time_i, remaining_time_i,
            elapsed_time_j, remaining_time_j;

        for (int i = 0; i < pos; i++)
        {
            taskQueue[i].priority = 1;
            clock_gettime(CLOCK_REALTIME, &current_time);
            timespec_subtraction(current_time, taskQueue[i].arrive_time, &elapsed_time_i);
            int_timespec_subtraction(taskQueue[i].t.timelimit, elapsed_time_i, &remaining_time_i);

            if (timespec_to_double(remaining_time_i) < 0)
            {
                sync_log("TASK REMOVED\n", conf->log_file);
                removeTask(i);
                conf->removed_count++;
                i--;
                
                continue;
            }
            for (int j = 0; j < pos; j++)
            {
                if (i == j)
                    continue;

                clock_gettime(CLOCK_REALTIME, &current_time);
                timespec_subtraction(current_time, taskQueue[j].arrive_time, &elapsed_time_j);
                int_timespec_subtraction(taskQueue[i].t.timelimit, elapsed_time_j, &remaining_time_j);

                if (timespec_to_double(remaining_time_j) < 0)
                {
                    sync_log("TASK REMOVED\n", conf->log_file);
                    removeTask(j);
                    conf->removed_count++;
                    j--;
                    continue;
                }
                if (timespec_cmp(remaining_time_i, remaining_time_j) > 0)
                {
                    taskQueue[i].priority++;
                }

                else if (timespec_cmp(remaining_time_i, remaining_time_j) == 0 && i > j)
                {
                    taskQueue[i].priority++;
                }
            }
        }

        if (waiting)
        {
            op = insert;
        }
        else
        {
            op = dispatch;
        }
        pthread_cond_broadcast(&operation_cv);
        pthread_mutex_unlock(&operation_mutex);
    }
    pthread_exit(NULL);
    return NULL;
}

void *dispatcher()
{
    while (1)
    {
        pthread_mutex_lock(&operation_mutex);

        while (op != dispatch)
        {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            pthread_cond_wait(&operation_cv, &operation_mutex);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        }

        if (pos == 0)
        {
            op = insert;
            pthread_cond_broadcast(&operation_cv);
            pthread_mutex_unlock(&operation_mutex);
            continue;
        }

        pthread_mutex_lock(&(tm_shm->mutex));
        begin_shm_read();
        int num_servers = conf->num_servers;

        int available_cpus = conf->available_cpus;
        end_shm_read();

        while (available_cpus == 0)
        {
            waiting = true;
            op = insert;
            pthread_cond_broadcast(&operation_cv);
            pthread_mutex_unlock(&operation_mutex);
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            pthread_cond_wait(&(tm_shm->cv), &(tm_shm->mutex));
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            begin_shm_read();
            available_cpus = conf->available_cpus;
            end_shm_read();
        }
        if (waiting)
        {
            pthread_mutex_lock(&operation_mutex);

            while (op == schedule)
            {
                pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
                pthread_cond_wait(&operation_cv, &operation_mutex);
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            }

            op = dispatch;
            pthread_cond_signal(&operation_cv);
        }
        pthread_mutex_unlock(&(tm_shm->mutex));
        waiting = false;

        int i = 0;

        for (int j = 0; j < pos; j++)
        {
            if (taskQueue[j].priority < taskQueue[i].priority)
                i = j;
        }
        queuedTask task = taskQueue[i];
        int aux_tempo = 0, aux_vcpu = 0, value;
        struct timespec current_time, elapsed_time, remaining_time, execution_time;

        for (int j = 0; j < num_servers; j++)
        {
            begin_shm_read();
            int perf_level = servers[j].performance_lvl;
            end_shm_read();

            for (int k = 0; k < perf_level; k++)
            {
                begin_shm_read();
                int speed = servers[j].vcpus[k].speed;
                char *name = servers[j].name;
                end_shm_read();

                clock_gettime(CLOCK_REALTIME, &current_time);
                timespec_subtraction(current_time, task.arrive_time, &elapsed_time);
                int_timespec_subtraction(task.t.timelimit, elapsed_time, &remaining_time);

                double exec = ((double)(task.t.mi / 1000) / (double)speed);
                double_to_timespec(exec, &execution_time);

                if (timespec_cmp(execution_time, remaining_time) < 0)
                {
                    aux_tempo++;
                    sem_getvalue(&vcpu_sems[j * 2 + k], &value);
                    if (value == 0)
                    {
                        aux_vcpu++;
                        close(pipes[j][0]);
                        char send[DISPATCHEDSIZE];
                        int printed = snprintf(send, DISPATCHEDSIZE, "%s;%.6lf", task.t.id, timespec_to_double(execution_time));
                        for (int j = printed + 1; j < DISPATCHEDSIZE - 2; j++)
                            strcat(send, ";");

                        snprintf(logstring, LOGLEN, "TASK %s SELECTED FOR EXECUTION IN SERVER %s", task.t.id, name);
                        write(pipes[j][1], &send, DISPATCHEDSIZE);
                        begin_shm_write();
                        conf->available_cpus--;
                        conf->task_count++;
                        servers[j].executed_count++;
                        conf->wait_time += timespec_to_double(elapsed_time);
                        end_shm_write();
                        sync_log(logstring, conf->log_file);
                        sem_post(&vcpu_sems[j * 2 + k]);
                        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
                        removeTask(i);
                        break;
                    }
                }
                if (aux_vcpu > 0)
                    break;
            }
            if (aux_vcpu > 0)
                break;
        }
        if (aux_vcpu == 0)
        {
            sync_log("TASK REMOVED DUE TO LACK OF TIME TO EXECUTE", conf->log_file);
            removeTask(i);
            conf->removed_count++;
        }
        pthread_mutex_unlock(&operation_mutex);
    }
    pthread_exit(NULL);
    return NULL;
}

void sigint_task(){
    signal(SIGINT, SIG_IGN);
	sync_log("SIMULATOR WAITING FOR LAST TASKS TO FINISH",conf->log_file);
	tm_shm->cv.__data.__wrefs = 0;
	operation_cv.__data.__wrefs = 0;
	pthread_cancel(threads[0]);
	pthread_cancel(threads[1]);
	
	for (int i = 0; i < num_servers; i++) {
		kill(ids[i], SIGUSR1);
    }
    for (int i = 0; i < num_servers; i++) {
		wait(NULL);
    }
    
    for (int i = 0;i < num_servers*2; i++){
		sem_destroy(&(vcpu_sems[i]));
	}
	tm_shm->cv.__data.__wrefs = 0;
	operation_cv.__data.__wrefs = 0;
	pthread_mutex_destroy(&operation_mutex);
	pthread_cond_destroy(&operation_cv);
	pthread_cond_destroy(&(tm_shm->cv));
	for(int i = 0; i < pos; i++){
		snprintf(logstring,LOGLEN,"TASK %s WAS NOT EXECUTED",taskQueue[i].t.id);
		sync_log(logstring,conf->log_file);
	}
	close(fd);
	munmap(0,sizeof(taskmanager_shm) + num_servers * 2 * sizeof(sem_t));
	shm_unlink(TASK_SHMNAME);
	exit(0);
}

void removeTask(int a)
{
    taskQueue[a] = taskQueue[pos - 1];
    pos--;
    pthread_mutex_lock(monitor_mutex);
    begin_shm_write();
    conf->percent_filled -= (float)1/(float)queue_pos;
    pthread_cond_signal(monitor_cv);
    pthread_mutex_unlock(monitor_mutex);
    end_shm_write();
}

int timespec_cmp(struct timespec t1, struct timespec t2)
{
    if (t1.tv_sec > t2.tv_sec)
        return 1;

    if (t1.tv_sec < t2.tv_sec)
        return -1;

    if (t1.tv_sec == t2.tv_sec)
    {
        if (t1.tv_nsec > t2.tv_nsec)
            return 1;

        if (t1.tv_nsec < t2.tv_nsec)
            return -1;
    }
    return 0;
}

void timespec_subtraction(struct timespec t1, struct timespec t2, struct timespec *t3)
{
    t3->tv_nsec = t1.tv_nsec - t2.tv_nsec;
    t3->tv_sec = t1.tv_sec - t2.tv_sec;

    if (t3->tv_sec > 0 && t3->tv_nsec < 0)
    {
        t3->tv_nsec += 1000000000;
        t3->tv_sec--;
    }

    else if (t3->tv_sec < 0 && t3->tv_nsec > 0)
    {
        t3->tv_nsec -= 1000000000;
        t3->tv_sec++;
    }
}

void int_timespec_subtraction(int sec, struct timespec t1, struct timespec *t2)
{
    t2->tv_sec = sec - t1.tv_sec - 1;
    t2->tv_nsec = 1000000000 - t1.tv_nsec;
}

void double_to_timespec(double d, struct timespec *t)
{
    double i;
    double frac = modf(d, &i);
    t->tv_sec = i;
    t->tv_nsec = frac * 1000000000;
}

double timespec_to_double(struct timespec t)
{
    return (double)t.tv_sec + (double)(t.tv_nsec / 1000000000);
}
