#ifndef DRAWING_H
#define DRAWING_H

#include <SDL.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_LINE_COUNT 100
#define PALETTE_SIZE 6

typedef struct {
    Uint8 r, g, b;
} Color;

typedef struct {
    int x1, y1, x2, y2;
    Color color;
} Line;

typedef struct {
    int x, y, width, height;
    Color color;
} Rectangle;

/**
 * Inicializálja az SDL-t és létrehozza az ablakot.
 */
int init_SDL(SDL_Window** window, SDL_Renderer** renderer);

/**
 * Kirajzolja az összes szakaszt.
 */
void draw_lines(SDL_Renderer* renderer, Line lines[], int line_count);

/**
 * Kirajzolja az összes téglalapot.
 */
void draw_rectangles(SDL_Renderer* renderer, Rectangle rects[], int rect_count);

/**
 * Paletta kirajzolása a képernyő bal oldalán.
 */
void draw_palette(SDL_Renderer* renderer, Color palette[], int selected_color);

/**
 * SDL bezárása.
 */
void close_SDL(SDL_Window* window, SDL_Renderer* renderer);

#endif // DRAWING_H