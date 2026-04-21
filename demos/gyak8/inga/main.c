#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define PI 3.14159265

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Inga animáció", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    bool running = true;
    Uint64 start = SDL_GetTicks();
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }

        Uint64 now = SDL_GetTicks();
        float t = (now - start) / 1000.0f;

        // Szinuszos mozgás (3 másodperc oda vagy vissza)
        float angle_deg = 40.0f * sin(PI * t / 3.0f);
        float angle_rad = angle_deg * PI / 180.0f;

        // Inga hossza és középpont
        int cx = WINDOW_WIDTH / 2;
        int cy = 100;
        int length = 200;

        int x2 = cx + (int)(length * sin(angle_rad));
        int y2 = cy + (int)(length * cos(angle_rad));

        // Rajzolás
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawLine(renderer, cx, cy, x2, y2);
        SDL_Rect bob = {x2 - 10, y2 - 10, 20, 20};
        SDL_RenderFillRect(renderer, &bob);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}