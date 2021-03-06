CC=gcc
CFLAGS=-O2
INCLUDES=
LDFLAGS=-libverbs
LIBS=-lpthread -lrdmacm -lpmem

SRCS=main.c client.c config.c ib.c server.c setup_ib.c sock.c latency.c NVM.c client4write.c server4write.c client4persist.c server4persist.c client4lat.c server4lat.c client4ddio.c server4ddio.c
OBJS=$(SRCS:.c=.o)
PROG=rdma_test

all: $(PROG)

debug: CFLAGS=m -g -DDEBUG
debug: $(PROG)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
	$(RM) *.o

clean:
	$(RM) *.o *~ $(PROG)
