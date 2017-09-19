CC=gcc
CFLAGS=-Wall -O3 -funroll-loops -fforce-addr
LDFLAGS=
LIBS=-lm

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

all: main.o
	gcc $(CFLAGS) $(LDFLAGS) -o lisp $^ $(LIBS)

clean:
	rm -rf *.o

