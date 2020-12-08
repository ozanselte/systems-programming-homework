CC = gcc
CFLAGS = -Wall -pedantic-errors -std=gnu99
CFLAGS += -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE
CFLAGS += -D_FILE_OFFSET=64
LIBS = -lm -lrt -pthread
DEPS = main.h helper.h structures.h florish.h
OBJ = main.o helper.o structures.o florist.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

all: hw5

hw5: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f *.o hw5
