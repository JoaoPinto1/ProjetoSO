#include "task_manager.h"

pthread_mutex_t operation_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t scheduled_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fastvcpu_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t empty_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t vcpu_cv = PTHREAD_COND_INITIALIZER;
pthread_cond_t operation_cv = PTHREAD_COND_INITIALIZER;
pthread_cond_t monitor_cv = PTHREAD_COND_INITIALIZER;
sem_t *vcpu_sems;
int **pipes, pos, v;

void taskmanager()
{
    sync_log("PROCESS TASK MANAGER CREATED", conf->log_file);
    begin_shm_read();
    int num_servers = conf->num_servers, queue_pos = conf->queue_pos;
    end_shm_read();
    pipes = (int **)malloc(num_servers * 2 * sizeof(int));
    
    for (int i = 0; i < num_servers; i++)
    {
        pipe(pipes[i]);
    }

    int task_fd = shm_open("EDGESERVERS", O_CREAT | O_RDWR, 0666);
    ftruncate(task_fd, num_servers * 2 * sizeof(sem_t));
    vcpu_sems = (sem_t *)mmap(0, num_servers * 2 * sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, task_fd, 0);

    for (int i = 0; i < conf->num_servers * 2; i++)
    {
        sem_init(&(vcpu_sems[i]), 1, 0);
    }

    op = insert;
    taskQueue = (queuedTask *)malloc(sizeof(queuedTask) * queue_pos);
    int fd = open("TASK_PIPE", O_RDWR);
    if (fd == -1)
    {
        sync_log("ERROR OPENING TASK_PIPE", conf->log_file);
        exit(0);
    }
    for (int i = 0; i < conf->num_servers; i++)
    {
        if (fork() == 0)
        {
            edgeserver(&servers[i], i, pipes[i]);
            exit(0);
        }
    }
    int id[2];
    pthread_t threads[2];
    id[0] = 0;
    pthread_create(&threads[0], NULL, scheduler, &id[0]);
    id[1] = 1;
    pthread_create(&threads[1], NULL, dispatcher, &id[1]);

    char string[SIZETASK], tid[SIZETASK], buffer[SIZETASK - 4];
    pos = 0;

    while (1)
    {

        int to_read, mi, timeLimit, len;
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
            sync_log("EXIT", conf->log_file);
            break;
        }

        if ((len = read(fd, buffer, 1)) == -1)
        {
            sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
            exit(0);
        }
        buffer[len] = '\0';
        if (strcmp(string, "STATS") == 0)
        {
            sync_log("STATS", conf->log_file);
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
            while (op == schedule || pos == conf->queue_pos)
            {
                pthread_cond_wait(&operation_cv, &operation_mutex);
            }
	    op = insert;
	    pthread_cond_broadcast(&operation_cv);
            strcpy(taskQueue[pos].t.id, tid);
            taskQueue[pos].t.mi = mi;
            taskQueue[pos].t.timelimit = timeLimit;
            taskQueue[pos].priority = 0;
            taskQueue[pos].arrive_time = (double)clock() / CLOCKS_PER_SEC;
            printf("TASK INSERIDA NA LISTA: %s«\n", taskQueue[pos].t.id);
            pos++;

            op = schedule;
            pthread_cond_signal(&monitor_cv);
            pthread_cond_broadcast(&operation_cv);
            pthread_mutex_unlock(&operation_mutex);
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
	sync_log("scheduler", conf->log_file);
    while (1)
    {

        pthread_mutex_lock(&operation_mutex);
        while (op != schedule)
        {
            pthread_cond_wait(&operation_cv, &operation_mutex);
        }
        for (int i = 0; i < pos; i++)
        {
            taskQueue[i].priority = 1;
            
            for (int j = 0; j < pos; j++)
            {
                if (taskQueue[i].t.timelimit > taskQueue[j].t.timelimit)
                {
                    taskQueue[i].priority++;
                }
                
                else if (taskQueue[i].t.timelimit == taskQueue[j].t.timelimit && i > j)
                {
                    taskQueue[i].priority++;
                }
            }
        }
        printf("prioridades ajustadas\n");
        op = dispatch;
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
            pthread_cond_wait(&operation_cv, &operation_mutex);
        }
        queuedTask t_aux;
        int i = 0;
        for (; i < pos && taskQueue[i].priority != 1; i++);

        t_aux = taskQueue[i];

        int aux_tempo = 0, aux_vcpu = 0, value;
        begin_shm_read();
        int num_servers = conf->num_servers;
        end_shm_read();
        
        for (int j = 0; j < num_servers && aux_vcpu == 0; j++)
        {
            begin_shm_read();
            int perf_level = servers[j].performance_lvl;
            end_shm_read();
            
            for (int k = 0; k < perf_level; k++)
            {
            	begin_shm_read();
            	int speed = servers[j].vcpus[k].speed;
            	end_shm_read();
            	
                // tempo_restante = t_max - (t_presente - t_arrive)
                double remaining = t_aux.t.timelimit - ((double)(clock() / CLOCKS_PER_SEC) - t_aux.arrive_time);
                double execution = (double)((t_aux.t.mi / 1000) / speed);
                if (execution < remaining)
                {
                    printf("%ld\n", remaining - execution);
                    aux_tempo++;
                    sem_getvalue(&vcpu_sems[j * 2 + k], &value);
                    if (value == 0)
                    {
                        aux_vcpu++;
                        close(pipes[j][0]);
                        double send = remaining;
                        write(pipes[j][1], &send, sizeof(double));
                        printf("Enviada task %s para VCPU %d do server %d\n", t_aux.t.id, k, j);
                        sem_post(&vcpu_sems[j * 2 + k]);
                        removeTask(i);
                        pthread_cond_signal(&monitor_cv);
                        break;
                    }
                }
            }
            }
            if (aux_tempo == 0)
            {
                sync_log("TASK REMOVED DUE TO LACK OF TIME TO EXECUTE", conf->log_file);
            }
            else if (aux_vcpu == 0)
            {
                sync_log("NO VCPU AVAILABLE AT THE MOMENT", conf->log_file);
            }
        op = insert;
        pthread_cond_broadcast(&operation_cv);
        pthread_mutex_unlock(&operation_mutex);
    }
    pthread_exit(NULL);
    return NULL;
}

void removeTask(int a)
{
    taskQueue[a] = taskQueue[pos - 1];
    pos--;
}
/*
void edgeserver(edgeServer server, int num, pthread_mutex_t idle_mutex, pthread_cond_t idle_cv)
{
    idle_mutex = PTHREAD_MUTEX_INITIALIZER;
    idle_cv = PTHREAD_COND_INITIALIZER;
    int v1, v2;
    char string[40];
    snprintf(string, 39, "%s READY", server.name);
    sync_log(string, conf->log_file);

    pthread_t threads[2];
    int id[2];

    for (int i = 0; i < 2; i++)
    {
        id[i] = 10 * num + i;
        pthread_create(&threads[i], NULL, workercpu, &id[i]);
    }
    
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
        sem_getvalue(&vcpu_sems[num * 2], &v1);
        sem_getvalue(&vcpu_sems[num * 2 + 1], &v2);
        while ( v1 != 0 && v2 != 0) {
            pthread_cond_wait
    }
        
    for (int i = 0; i < 2; i++)
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
}*/

