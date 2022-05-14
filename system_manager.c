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
#include "maintenance_manager.h"

void *shm_pointer;
configs *conf;
edgeServer *servers;
readwrite_lock *rdwr_lock;

int shm_fd, l, msgid;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Not enough arguments\n");
        exit(-1);
    }
    l = open(LOGFILE, O_WRONLY | O_CREAT);

    logfunc("OFFLOAD SIMULATOR STARTING", l);
    int queuePos, maxWait, num, offset = 0, i;
    const char *filename = argv[1];

    FILE *f = fopen(filename, "r");
    
    if (!f)
    {
        exit(EXIT_FAILURE);
    }
    fscanf(f, "%d %d %d", &queuePos, &maxWait, &num);

    if (num < 2)
    {
        logfunc("Edge servers insuficientes", l);
        exit(EXIT_FAILURE);
    }
    if (mkfifo("TASK_PIPE", O_CREAT | O_EXCL | 0777) == -1)
    {
        if (errno != EEXIST)
        {
            logfunc("Could not create named pipe\n", l);
            return 1;
        }
    }
    
    if ((msgid = msgget(IPC_PRIVATE, IPC_CREAT | 0666)) == -1)
    {
        sync_log("ERROR READING FROM TASK_PIPE", conf->log_file);
        exit(0);
    }
   
    close(l);
  
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shm) + num * sizeof(edgeServer));

    shm_pointer = mmap(0, sizeof(shm) + num * sizeof(edgeServer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, offset);
    shm *shm_struct = (shm *) shm_pointer;
    conf = &(shm_struct->c);

    conf->max_wait = maxWait;
    conf->queue_pos = queuePos;
    conf->num_servers = num;
    conf->flag_servers = 1;
    conf->msgid = msgid;
    conf->log_file = open(LOGFILE, O_WRONLY | O_CREAT);
    conf->available_cpus = num;
    
    rdwr_lock = &(shm_struct->l);
    rdwr_lock->b = 0;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    
    pthread_condattr_t attr_cv;
    pthread_condattr_init(&attr_cv);
    pthread_condattr_setpshared(&attr_cv, PTHREAD_PROCESS_SHARED);
    
    pthread_mutex_init(&(rdwr_lock->read_mutex), &attr);
    pthread_mutex_init(&(conf->log_mutex), &attr);
    pthread_mutex_init(&(rdwr_lock->global_mutex), &attr);
    
    offset += sizeof(shm);
    servers = (edgeServer *)(shm_pointer + offset);
    
    for (i = 0; i < num; i++)
    {
        fscanf(f, " %[^,],%d,%d", servers[i].name, &(servers[i].vcpus[0].speed), &(servers[i].vcpus[1].speed));
        servers[i].performance_lvl = 1;
        if (servers[i].vcpus[0].speed < servers[i].vcpus[1].speed)
        {
            int aux = servers[i].vcpus[0].speed;
            servers[i].vcpus[0].speed = servers[i].vcpus[1].speed;
            servers[i].vcpus[1].speed = aux;
        }
    }
    fclose(f);
    
    sync_log("SHARED MEMORY CREATED", conf->log_file);

    for (i = 0; i < 3; i++)
    {
        if (fork() == 0)
        {
            switch (i)
            {

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
    for (i = 0; i < 3; i++)
    {
        wait(NULL);
    }
    shm_unlink(SHM_NAME);
    sync_log("SIMULATOR CLOSING", conf->log_file);
    close(conf->log_file);
    return 0;
}
