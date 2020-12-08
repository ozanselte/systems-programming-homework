CC = gcc
CFLAGS = -Wall -pedantic-errors -std=gnu99 -g
CFLAGS += -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE
CFLAGS += -D_FILE_OFFSET=64
LIBS = -lm -lrt -pthread
DEPS = client.h server.h helper.h graph.h workers.h
CLIENT_OBJ = client.o helper.o
SERVER_OBJ = server.o helper.o graph.o workers.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

all: client server

client: $(CLIENT_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server: $(SERVER_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f *.o client server