APP_NAME = museum

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS = -Llib -lobj -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lopengl32 -lm

SRC = src/main.c src/app.c src/camera.c src/scene_core.c src/scene_loader.c src/scene_render.c src/scene_interaction.c src/texture.c src/utils.c src/help.c src/csv.c

all:
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(APP_NAME).exe

linux:
	$(CC) $(CFLAGS) $(SRC) -Llib -lobj -lSDL2 -lSDL2_image -lGL -lm -o $(APP_NAME)

clean:
	del /q *.exe 2>nul || exit 0