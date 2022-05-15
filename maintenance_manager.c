#include "maintenance_manager.h"

void maintenance()
{
    srand(getpid());
    
    sync_log("PROCESS MAINTENANCE MANAGER CREATED", conf->log_file);
    begin_shm_read();
    int id = conf->msg_id, n = conf->num_servers;
    end_shm_read();
    msg m;
    while (1) 
    {
        printf("snoozin'\n");
        sleep(((float)rand()/(float)RAND_MAX) * 4 + 1);
        
        printf("woke up\n");
        float sleeptime = ((float)rand()/(float)RAND_MAX) * 4 + 1;
        strcpy(m.payload, "MAINT");
        long serv = rand() % n + 1;
    	m.msgtype = serv;
    	printf("sending message to %ld\n", serv - 1);
        msgsnd(id, &m, sizeof(msg) - sizeof(long), 0);
        msgrcv(id, &m, sizeof(msg) - sizeof(long), serv, 0);
        if (strcmp(m.payload, "OK") != 0) 
        {
            printf("SERVER %ld NOT READY FOR MAINTENANCE\n", serv - 1);
        }
        
        else 
        {
            printf("Maintenance for server %ld! schleepin'\n", serv - 1);
            sleep(sleeptime);
            strcpy(m.payload, "READY");
            msgsnd(id, &m, sizeof(msg) - sizeof(long), 0);
        }
   }
}
