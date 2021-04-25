CC=gcc
CFLAGS=-g -Wall -Wextra -Og

all: server client

server: server.c common.h
	$(CC) -o $@ $< $(CFLAGS) -lSDL2 -pthread

client: client.c common.h
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -f client server

.PHONY: all clean
