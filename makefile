CC	= gcc
OBJS	= system_manager.o log.o
PROG	= offload_manager
CFLAGS	= -Wall -pthread -lrt -g
all:	$(PROG)

clean:
	rm $(OBJS) *~ $(PROG)
$(PROG):	$(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $@
.c.o:
	$(CC) $< $(CFLAGS) -c -o $@

mobile_node.o:	mobile_node.c
	gcc mobile_node.c -o mobile_node

system_manager.o:	system_manager.c log.c log.h
log.o:	log.c log.h

