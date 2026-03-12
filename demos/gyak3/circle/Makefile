CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS = -lSDL2 -lm

all: main

main: main.o circle.o
	$(CC) -o main main.o circle.o $(LDFLAGS)

main.o: main.c circle.h
	$(CC) $(CFLAGS) -c main.c

circle.o: circle.c circle.h
	$(CC) $(CFLAGS) -c circle.c

clean:
	rm -f *.o main