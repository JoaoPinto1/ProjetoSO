#include "shm.h"


void monitor()
{
	sync_log("PROCESS MONITOR CREATED", conf->log_file);
	// por omissão os edge servers irão estar em normal performance com apenas um vcpu ativo
	
	while (1)
	{
		pthread_mutex_lock(monitor_mutex);
		while (conf->percent_filled < 0.8)
		{
			pthread_cond_wait(monitor_cv, monitor_mutex);
			printf("percent filled: %f\n",conf->percent_filled);
		}
		conf->flag_servers = 2;
		pthread_mutex_lock(flag_mutex);
		pthread_cond_signal(flag_cv);
		pthread_mutex_unlock(flag_mutex);
		sync_log("MONITOR ENHANCE THE EDGE SERVERS PERFORMANCE TO HIGH", conf->log_file);
		while (conf->percent_filled > 0.2){
			pthread_cond_wait(monitor_cv, monitor_mutex);
		}
		conf->flag_servers = 1;
		pthread_mutex_lock(flag_mutex);
		pthread_cond_signal(flag_cv);
		pthread_mutex_unlock(flag_mutex);
		sync_log("MONITOR LOWERS THE EDGE SERVERS PERFORMANCE TO NORMAL", conf->log_file);
		pthread_mutex_unlock(monitor_mutex);
	}
	
}
