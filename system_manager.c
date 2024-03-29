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
pthread_mutex_t *monitor_mutex;
pthread_mutex_t *flag_mutex;
pthread_cond_t *monitor_cv;
pthread_cond_t *flag_cv;
pid_t ids[3];
void sigint();

int shm_fd, l, msgid, num;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Not enough arguments\n");
        exit(-1);
    }
    l = open(LOGFILE, O_WRONLY | O_CREAT);

    logfunc("OFFLOAD SIMULATOR STARTING", l);
    
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGINT);
    sigdelset(&set,SIGTSTP);
    sigdelset(&set,SIGUSR1);
    sigprocmask(SIG_BLOCK,&set,NULL);
    
    signal(SIGINT,sigint);
    
    int queuePos, maxWait, offset = 0, i;
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
    conf->percent_filled = 0;
    conf->task_count = 0;
    conf->wait_time = 0;
    conf->available_cpus = num;
    conf->msg_id = msgid;
    conf->task_count = 0;
    conf->wait_time = 0;
    conf->removed_count = 0;
    conf->log_file = open(LOGFILE, O_WRONLY | O_CREAT);
    monitor_mutex = &(conf->monitor_mutex);
    monitor_cv = &(conf->monitor_cv);
    flag_mutex = &(conf->flag_mutex);
    flag_cv = &(conf->flag_cv);
    
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
    pthread_mutex_init(monitor_mutex,&attr);
    pthread_cond_init(monitor_cv,&attr_cv);
    pthread_mutex_init(flag_mutex,&attr);
    pthread_cond_init(flag_cv,&attr_cv);
    
    offset += sizeof(shm);
    servers = (edgeServer *)(shm_pointer + offset);
    
    for (i = 0; i < num; i++)
    {
        fscanf(f, " %[^,],%d,%d", servers[i].name, &(servers[i].vcpus[0].speed), &(servers[i].vcpus[1].speed));
        servers[i].performance_lvl = 1;
        servers[i].executed_count = 0;
        servers[i].maintenance_count = 0;
        if (servers[i].vcpus[0].speed < servers[i].vcpus[1].speed)
        {
            int aux = servers[i].vcpus[0].speed;
            servers[i].vcpus[0].speed = servers[i].vcpus[1].speed;
            servers[i].vcpus[1].speed = aux;
        }
    }
    fclose(f);
    
    sync_log("SHARED MEMORY CREATED", conf->log_file);

    if ((ids[0] = fork()) == 0) {
    	signal(SIGTSTP, SIG_IGN);
    	signal(SIGINT, SIG_IGN);
        monitor();
        exit(0);
    }
    
    if ((ids[1] = fork()) == 0) {
    	signal(SIGTSTP, SIG_IGN);
    	signal(SIGINT, SIG_IGN);
        maintenance();
        exit(0);
    }
    
    if ((ids[2] = fork()) == 0) {
    	signal(SIGTSTP, SIG_IGN);
    	signal(SIGINT, SIG_IGN);
        taskmanager();
        exit(0);
    }
    struct sigaction sa;
    sa.sa_handler = &stats;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL);
    for (i = 0; i < 3; i++)
    {
        wait(NULL);
    }
    shm_unlink(SHM_NAME);
    sync_log("SIMULATOR CLOSING", conf->log_file);
    close(conf->log_file);
    return 0;
}

void sigint(){
	sync_log("SIGNAL SIGINT RECIEVED", conf->log_file);
	for (int i = 0; i < 3; i++)
    {
        kill(ids[i], SIGUSR1);
    }
    
	for (int i = 0; i < 3; i++)
    {
        wait(NULL);
    }
    
    stats();
	monitor_cv->__data.__wrefs = 0;
	flag_cv->__data.__wrefs = 0;
    pthread_mutex_destroy(&(rdwr_lock->read_mutex));
    pthread_mutex_destroy(&(conf->log_mutex));
    pthread_mutex_destroy(&(rdwr_lock->global_mutex));
    pthread_mutex_destroy(monitor_mutex);
    pthread_cond_destroy(monitor_cv);
    pthread_mutex_destroy(flag_mutex);
    pthread_cond_destroy(flag_cv);
	munmap(0, sizeof(shm) + num * sizeof(edgeServer));
	shm_unlink(SHM_NAME);
	sync_log("SIMULATOR CLOSING", conf->log_file);
	close(conf->log_file);
	pthread_mutex_destroy(&(conf->log_mutex));
	exit(0);
}
