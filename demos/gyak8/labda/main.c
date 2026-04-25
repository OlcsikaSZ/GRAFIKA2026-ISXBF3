#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>

#define WIDTH 800
#define HEIGHT 600
#define BALL_RADIUS 32
#define GRAVITY 980.0
#define TIME_STEP 0.016
#define DAMPING 0.8
#define INITIAL_Y 100.0

typedef struct {
    double y;
    double v;
} Ball;

void simulate(Ball *ball, bool ideal) {
    ball->v += GRAVITY * TIME_STEP;
    ball->y += ball->v * TIME_STEP;

    if (ball->y >= HEIGHT - BALL_RADIUS) {
        ball->y = HEIGHT - BALL_RADIUS;
        ball->v = -(ideal ? ball->v : ball->v * DAMPING);
    }
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0 || IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Pattogó labda",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Surface *image = IMG_Load("ball.png");
    if (!image) {
        fprintf(stderr, "Nem sikerült betölteni a képet: %s\n", IMG_GetError());
        return 1;
    }
    SDL_Texture *ball_texture = SDL_CreateTextureFromSurface(renderer, image);
    SDL_FreeSurface(image);

    Ball ball = { .y = INITIAL_Y, .v = 0.0 };
    bool quit = false, ideal = false;
    SDL_Event event;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                quit = true;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE)
                    ideal = !ideal;
            }
        }

        simulate(&ball, ideal);

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        SDL_Rect dst = {WIDTH / 2 - BALL_RADIUS, (int)(ball.y - BALL_RADIUS), BALL_RADIUS * 2, BALL_RADIUS * 2};
        SDL_RenderCopy(renderer, ball_texture, NULL, &dst);

        SDL_RenderPresent(renderer);
        SDL_Delay((int)(TIME_STEP * 1000));
    }

    SDL_DestroyTexture(ball_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}