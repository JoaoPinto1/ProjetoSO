CC	= gcc
OBJS	= shm.o log.o monitor.o task_manager.o system_manager.o maintenance_manager.o edge_server.o
PROG	= offload_manager
CFLAGS	= -Wall -pthread -lrt -g
all:	$(PROG) mobile_node

clean:
	rm $(OBJS) *~ $(PROG)
$(PROG):	$(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $@
.c.o:
	$(CC) $< $(CFLAGS) -c -o $@

mobile_node:	mobile_node.c
	$(CC) mobile_node.c $(CFLAGS) -o $@


maintenance_manager.o:	maintenance_manager.c maintenance_manager.h
shm.o:	shm.c shm.h
log.o:	log.c log.h
edge_server.o:	edge_server.c edge_server.h task_shm.h
task_manager.o:	shm.c shm.h task_manager.h task_manager.c task.h edge_server.c edge_server.h task_shm.h
monitor.o:	shm.c shm.h monitor.c monitor.h
system_manager.o:	shm.c shm.h system_manager.c log.c log.h shm.h monitor.h monitor.c task_manager.h task_manager.c maintenance_manager.c maintenance_manager.h

