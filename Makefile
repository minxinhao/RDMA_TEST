CC=gcc
CFLAGS=-O2
INCLUDES=
LDFLAGS=-libverbs
LIBS=-lpthread -lrdmacm -lpmem

SRCS=main.c config.c ib.c setup_ib.c sock.c client.c server.c client_write.c server4write.c
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
