#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "circle.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_CIRCLE_COUNT 10

Circle circles[MAX_CIRCLE_COUNT];
int circle_count = 0;

/**
 * Véletlenszerű szín generálása
 */
Color random_color() {
    Color color;
    color.r = rand() % 256;
    color.g = rand() % 256;
    color.b = rand() % 256;
    return color;
}

/**
 * Kör kirajzolása SDL-nek
 */
void draw_circle(SDL_Renderer* renderer, Circle* circle) {
    SDL_SetRenderDrawColor(renderer, circle->color.r, circle->color.g, circle->color.b, 255);

    int step = 360 / 50;  // Közelítés 50 szakaszból
    for (int angle = 0; angle < 360; angle += step) {
        double x1 = circle->x + circle->radius * cos(angle * M_PI / 180);
        double y1 = circle->y + circle->radius * sin(angle * M_PI / 180);
        double x2 = circle->x + circle->radius * cos((angle + step) * M_PI / 180);
        double y2 = circle->y + circle->radius * sin((angle + step) * M_PI / 180);

        SDL_RenderDrawLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
    }
}

/**
 * "X" jelet rajzolunk a körök közepébe, ha a kurzor fölöttük van
 */
void draw_cross(SDL_Renderer* renderer, Circle* circle) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // Fehér "X"
    SDL_RenderDrawLine(renderer, circle->x - 5, circle->y - 5, circle->x + 5, circle->y + 5);
    SDL_RenderDrawLine(renderer, circle->x - 5, circle->y + 5, circle->x + 5, circle->y - 5);
}

/**
 * Ellenőrzi, hogy a kurzor a kör fölött van-e
 */
bool is_cursor_inside_circle(int mouse_x, int mouse_y, Circle* circle) {
    double dx = mouse_x - circle->x;
    double dy = mouse_y - circle->y;
    return (dx * dx + dy * dy) <= (circle->radius * circle->radius);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Circle Drawing",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH,
                                          WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    srand(time(NULL));

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN && circle_count < MAX_CIRCLE_COUNT) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                circles[circle_count].x = x;
                circles[circle_count].y = y;
                circles[circle_count].radius = rand() % 30 + 20;
                circles[circle_count].color = random_color();
                circle_count++;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Háttér fekete
        SDL_RenderClear(renderer);

        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);

        for (int i = 0; i < circle_count; i++) {
            draw_circle(renderer, &circles[i]);
            if (is_cursor_inside_circle(mouse_x, mouse_y, &circles[i])) {
                draw_cross(renderer, &circles[i]);
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}