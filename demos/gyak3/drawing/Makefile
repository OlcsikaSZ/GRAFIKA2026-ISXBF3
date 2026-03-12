# Fordító
CFLAGS = -Wall -Wextra -std=c11 $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)

# Fájlnevek
SRC = main.c drawing.c
OBJ = $(SRC:.c=.o)
EXEC = line_drawer

# Alapértelmezett cél: a program fordítása
all: $(EXEC)

# Az végrehajtható fájl létrehozása
$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LDFLAGS)

# Egyedi függvénykönyvtárak fordítása
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Futtatás
run: all
	./$(EXEC)

# Takarítás (fordított fájlok törlése)
clean:
	rm -f $(OBJ) $(EXEC)