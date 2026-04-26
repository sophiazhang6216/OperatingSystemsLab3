#Makefile for server and client genereated by Cursor
#prompt: can you make a makefile for server and client that compiles the code and runs the server and client

CC      := gcc
CFLAGS  := -Wall -g
LDFLAGS :=

HEADERS := shared_func.h rbtree.h

SHARED_OBJS := shared_func.o
SERVER_OBJS := server.o $(SHARED_OBJS)
CLIENT_OBJS := client.o $(SHARED_OBJS)

asan: CFLAGS += -fsanitize=address,leak
asan: LDFLAGS += -fsanitize=address,leak
asan: all

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
