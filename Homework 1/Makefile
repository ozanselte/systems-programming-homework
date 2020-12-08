CC = gcc
CFLAGS = -Wall -pedantic-errors -std=gnu99
LIBS = -lm
DEPS = x_funcs.h a_funcs.h b_funcs.h rosetta_fft.h
OBJ = x_funcs.o
A_OBJ = a_main.o a_funcs.o
B_OBJ = b_main.o b_funcs.o rosetta_fft.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

all: programA programB

programA: $(OBJ) $(A_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

programB: $(OBJ) $(B_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f *.o programA programB programC