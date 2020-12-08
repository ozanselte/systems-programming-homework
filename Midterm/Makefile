CC = gcc
CFLAGS = -Wall -pedantic-errors -std=gnu99 -g
CFLAGS += -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE
CFLAGS += -D_FILE_OFFSET=64
LIBS = -lm -lrt -pthread
DEPS = deps.h main.h deps_b.h main_b.h
OBJ = main.o deps.o
OBJBONUS = main_b.o deps_b.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

all: midterm bonus

midterm: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

bonus: $(OBJBONUS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f *.o midterm bonus