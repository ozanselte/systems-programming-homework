CC = gcc
CFLAGS = -Wall -pedantic-errors -std=gnu99 -g
CFLAGS += -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE
CFLAGS += -D_FILE_OFFSET=64
LIBS = -lm
DEPS = funcs.h processes.h defs_and_types.h
OBJ = main.o funcs.o processes.o svd.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

all: hw3

hw3: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f *.o hw3