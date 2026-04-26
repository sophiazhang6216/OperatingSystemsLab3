#Makefile for server and client genereated by Cursor
#prompt: can you make a makefile for server and client that compiles the code and runs the server and client

CC      := gcc
CFLAGS  := -Wall -g
LDFLAGS :=

SERVER_SRC := server.c
CLIENT_SRC := client.c

SHARED_SRCS := shared_func.c
HEADERS     := shared_func.h rbtree.h

BINS := server client

.PHONY: all clean

all: $(BINS)

server: $(SERVER_SRC) $(SHARED_SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_SRC) $(LDFLAGS)

client: $(CLIENT_SRC) $(SHARED_SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_SRC) $(LDFLAGS)

clean:
	rm -f $(BINS)
