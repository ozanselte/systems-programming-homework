CC = gcc
CFLAGS = -Wall -pedantic-errors -std=gnu99
LIBS = -lm
DEPS = funcs.h
OBJ = main.o funcs.o process1.o process2.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

all: hw2

hw2: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f *.o hw2