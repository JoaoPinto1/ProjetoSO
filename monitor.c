#include "monitor.h"


void monitor() {
	sync_log("PROCESS MONITOR CREATED FROM HEADER FILE!", conf->log_file);
	//por omissão os edge servers irão estar em normal performance com apenas um vcpu ativo
	conf->flag_servers = 'n';
	while(1){
		
	}
}
