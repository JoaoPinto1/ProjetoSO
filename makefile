CC	= gcc
OBJS	= system_manager.o log.o
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

system_manager.o:	system_manager.c log.c log.h
log.o:	log.c log.h

