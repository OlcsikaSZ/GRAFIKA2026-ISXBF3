CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LIBS = -lmingw32 -lSDL2main -lSDL2 -lm
SRC = main.c
TARGET = pendulum.exe

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	del $(TARGET)