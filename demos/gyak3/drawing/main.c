#include "drawing.h"

int main() {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    if (!init_SDL(&window, &renderer)) return 1;

    Line lines[MAX_LINE_COUNT];
    Rectangle rects[MAX_LINE_COUNT];
    int line_count = 0;
    int rect_count = 0;
    int drawing_mode = 0; // 0: Lines, 1: Rectangles

    Color palette[PALETTE_SIZE] = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0}, {255, 0, 255}, {0, 255, 255}
    };
    int selected_color = 0;

    int drawing = 0;
    int start_x, start_y;

    SDL_Event event;
    int running = 1;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.x < 50) {
                    selected_color = (event.button.y - 10) / 40;
                    if (selected_color < 0 || selected_color >= PALETTE_SIZE) {
                        selected_color = 0;
                    }
                } else {
                    start_x = event.button.x;
                    start_y = event.button.y;
                    drawing = 1;
                }
            } else if (event.type == SDL_MOUSEBUTTONUP && drawing) {
                if (drawing_mode == 0 && line_count < MAX_LINE_COUNT) {
                    lines[line_count++] = (Line){start_x, start_y, event.button.x, event.button.y, palette[selected_color]};
                } else if (drawing_mode == 1 && rect_count < MAX_LINE_COUNT) {
                    rects[rect_count++] = (Rectangle){start_x, start_y, event.button.x - start_x, event.button.y - start_y, palette[selected_color]};
                }
                drawing = 0;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    drawing_mode = !drawing_mode;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        draw_palette(renderer, palette, selected_color);

        if (drawing_mode == 0) {
            draw_lines(renderer, lines, line_count);
        } else {
            draw_rectangles(renderer, rects, rect_count);
        }

        SDL_RenderPresent(renderer);
    }

    close_SDL(window, renderer);
    return 0;
}