APP_NAME = museum

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Iinclude -Iext/obj/include -Iext/obj/include/obj
LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lopengl32 -lm

SRC = src/main.c src/app.c src/camera.c src/scene.c src/texture.c src/utils.c src/help.c src/csv.c
OBJ_SRC = ext/obj/src/model.c ext/obj/src/load.c ext/obj/src/info.c ext/obj/src/draw.c ext/obj/src/transform.c

all:
	$(CC) $(CFLAGS) $(SRC) $(OBJ_SRC) $(LDFLAGS) -o $(APP_NAME).exe

linux:
	$(CC) -Wall -Wextra -Wpedantic -Iinclude -Iext/obj/include \
		$(SRC) $(OBJ_SRC) -lSDL2 -lSDL2_image -lGL -lm -o $(APP_NAME)

clean:
	del /q *.exe 2>nul || exit 0