CC=gcc
CFLAGS=-Wall -Wextra -O3

all: server client

server: server.c common.h
	$(CC) -o $@ $< $(CFLAGS) -lSDL2 -pthread

client: client.c common.h
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -f client server

.PHONY: all clean
