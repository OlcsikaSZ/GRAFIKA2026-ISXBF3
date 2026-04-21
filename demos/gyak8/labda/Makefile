CC=gcc
CFLAGS=-IC:/SDL2/include -IC:/SDL2_image/include -O2 -Wall
LDFLAGS=-LC:/SDL2/lib -LC:/SDL2_image/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -mwindows
TARGET=bouncing_ball

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET).exe main.c $(LDFLAGS)

clean:
	del $(TARGET).exe