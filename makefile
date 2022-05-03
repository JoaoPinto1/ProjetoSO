CC	= gcc
OBJS	= log.o monitor.o task_manager.o system_manager.o
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


log.o:	log.c log.h
task_manager.o:	shm.h task_manager.h task_manager.c task.h
monitor.o:	shm.h monitor.c monitor.h
system_manager.o:	system_manager.c log.c log.h shm.h monitor.h monitor.c task_manager.h task_manager.c

