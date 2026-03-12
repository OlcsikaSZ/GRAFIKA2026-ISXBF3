#include "drawing.h"

int init_SDL(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 0;
    }

    *window = SDL_CreateWindow("Line Drawer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!*window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        return 0;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(*window);
        return 0;
    }

    return 1;
}

void draw_lines(SDL_Renderer* renderer, Line lines[], int line_count) {
    for (int i = 0; i < line_count; i++) {
        SDL_SetRenderDrawColor(renderer, lines[i].color.r, lines[i].color.g, lines[i].color.b, 255);
        SDL_RenderDrawLine(renderer, lines[i].x1, lines[i].y1, lines[i].x2, lines[i].y2);
    }
}

void draw_rectangles(SDL_Renderer* renderer, Rectangle rects[], int rect_count) {
    for (int i = 0; i < rect_count; i++) {
        SDL_SetRenderDrawColor(renderer, rects[i].color.r, rects[i].color.g, rects[i].color.b, 255);
        SDL_Rect rect = {rects[i].x, rects[i].y, rects[i].width, rects[i].height};
        SDL_RenderFillRect(renderer, &rect);
    }
}

void draw_palette(SDL_Renderer* renderer, Color palette[], int selected_color) {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        SDL_SetRenderDrawColor(renderer, palette[i].r, palette[i].g, palette[i].b, 255);
        SDL_Rect rect = {10, 10 + i * 40, 30, 30};
        SDL_RenderFillRect(renderer, &rect);

        if (i == selected_color) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
    }
}

void close_SDL(SDL_Window* window, SDL_Renderer* renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}