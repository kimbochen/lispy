CFLAGS = -std=c99 -Wall
LIB = -ledit -lm

all: parsing.o
	cc $(CFLAGS) parsing.o mpc.o $(LIB) -o parsing

debug: parsing.c
	cc $(CFLAGS) -g -c parsing.c
	cc $(CFLAGS) -g parsing.o mpc.o $(LIB) -o parsing

parsing.o: parsing.c
	cc $(CFLAGS) -c parsing.c

mpc.o: mpc.c
	cc $(CFLAGS) -c mpc.c

clean:
	rm -f parsing
