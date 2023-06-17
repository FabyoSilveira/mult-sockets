CC=gcc
CFLAGS=-pthread

all: user server

user: user.c
	$(CC) $(CFLAGS) -o user user.c

server: server.c
	$(CC) $(CFLAGS) -o server server.c

clean:
	rm -f user server