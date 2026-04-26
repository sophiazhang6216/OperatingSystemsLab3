CC      := gcc
CFLAGS  := -Wall -g
LDFLAGS :=

HEADERS := shared_func.h rbtree.h

SHARED_OBJS := shared_func.o
SERVER_OBJS := server.o $(SHARED_OBJS)
CLIENT_OBJS := client.o $(SHARED_OBJS)

BINS := server client

.PHONY: all clean

all: $(BINS)

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(BINS) *.o
