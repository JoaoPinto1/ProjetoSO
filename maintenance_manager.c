#include "maintenance_manager.h"

void sigint_maintenance();
char logprint[128];
void maintenance()
{
	signal(SIGUSR1,sigint_maintenance);
    srand(getpid());
    
    sync_log("PROCESS MAINTENANCE MANAGER CREATED", conf->log_file);
    begin_shm_read();
    int id = conf->msg_id, n = conf->num_servers;
    end_shm_read();
    msg m;
    while (1) 
    {
        sleep(((float)rand()/(float)RAND_MAX) * 4 + 1);
        
        float sleeptime = ((float)rand()/(float)RAND_MAX) * 4 + 1;
        strcpy(m.payload, "MAINT");
        long serv = rand() % n + 1;
    	m.msgtype = serv;
        msgsnd(id, &m, sizeof(msg) - sizeof(long), 0);
        msgrcv(id, &m, sizeof(msg) - sizeof(long), serv, 0);
        if (strcmp(m.payload, "OK") != 0) 
        {
            snprintf(logprint,LOGLEN,"SERVER %ld NOT READY FOR MAINTENANCE", serv - 1);
            sync_log(logprint,conf->log_file);
        }
        
        else 
        {
            snprintf(logprint,LOGLEN,"Maintenance for server %ld!", serv - 1);
            sync_log(logprint,conf->log_file);
            sleep(sleeptime);
            strcpy(m.payload, "READY");
            msgsnd(id, &m, sizeof(msg) - sizeof(long), 0);
        }
   }
}

void sigint_maintenance(){
	if (msgctl(conf->msg_id, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Message queue could not be deleted.\n");
		exit(EXIT_FAILURE);
	}
	exit(0);
}
