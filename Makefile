CFLAGS = -std=c99 -Wall
LIB = -ledit -lm

all: main.o
	cc $(CFLAGS) main.o mpc.o $(LIB) -o lispy

debug: main.o
	cc $(CFLAGS) -g main.o mpc.o $(LIB) -o lispy

main.o: main.c
	cc $(CFLAGS) -c main.c

mpc.o: mpc.c
	cc $(CFLAGS) -c mpc.c

clean:
	rm -f lispy
