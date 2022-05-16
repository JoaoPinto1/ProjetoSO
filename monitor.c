#include "shm.h"
void sigint_monitor();

void monitor()
{

	sync_log("PROCESS MONITOR CREATED", conf->log_file);
	signal(SIGUSR1,sigint_monitor);
	while (1)
	{
		pthread_mutex_lock(monitor_mutex);
		while (conf->percent_filled < 0.8)
		{
			pthread_cond_wait(monitor_cv, monitor_mutex);
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

void sigint_monitor(){
	exit(0);
}
