#include "monitor.h"

pthread_mutex_t monitor_mutex = PTHREAD_MUTEX_INITIALIZER;

void monitor()
{
	sync_log("PROCESS MONITOR CREATED FROM HEADER FILE!", conf->log_file);
	// por omissão os edge servers irão estar em normal performance com apenas um vcpu ativo
	conf->flag_servers = 1;
	while (1)
	{
		pthread_mutex_lock(&operation_mutex);
		while (pos < (int)(conf->queuePos * 0.8))
		{
			pthread_cond_wait(&monitor_cv, &monitor_mutex);
		}
		conf->flag_servers = 2;
		for (int i = 0; i < conf->num_servers; i++){
			servers[i].performance_lvl = 2;
		}
		sync_log("MONITOR ENHANCE THE EDGE SERVERS PERFORMANCE TO HIGH", conf->log_file);
		while (pos < (int)(conf->queuePos*0.2)){
			pthread_cond_wait(&monitor_cv, &monitor_mutex);
		}
		conf->flag_servers = 1;
		for (int i = 0; i < conf->num_servers; i++){
			servers[i].performance_lvl = 1;
		}
		sync_log("MONITOR LOWERS THE EDGE SERVERS PERFORMANCE TO NORMAL", conf->log_file);
		pthread_mutex_unlock(&operation_mutex);
	}
}
