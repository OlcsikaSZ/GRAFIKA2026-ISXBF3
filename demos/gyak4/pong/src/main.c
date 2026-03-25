#include <SDL2/SDL.h>
#include <stdbool.h>
#include "ball.h"
#include "paddle.h"
#include "game.h"

// Define key constants for ball resizing
#define KEY_INCREASE_SIZE SDLK_PLUS
#define KEY_DECREASE_SIZE SDLK_MINUS

// Define the initial paddle positions
#define LEFT_PADDLE_X 50
#define RIGHT_PADDLE_X 750
#define PADDLE_WIDTH 20
#define PADDLE_HEIGHT 100

int main()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Ball ball;
    init_ball(&ball, 400, 300);

    float paddle_left_y = 250;
    float paddle_right_y = 250;

    int left_player_score = 0;
    int right_player_score = 0;

    bool running = true;
    Uint32 last_time = SDL_GetTicks();

    while (running) {
        Uint32 current_time = SDL_GetTicks();
        double elapsed_time = (current_time - last_time) / 1000.0;
        last_time = current_time;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                        paddle_left_y -= 10;
                        break;
                    case SDLK_s:
                        paddle_left_y += 10;
                        break;
                    case SDLK_UP:
                        paddle_right_y -= 10;
                        break;
                    case SDLK_DOWN:
                        paddle_right_y += 10;
                        break;
                    case KEY_INCREASE_SIZE:
                        resize_ball(&ball, 5);
                        break;
                    case KEY_DECREASE_SIZE:
                        resize_ball(&ball, -5);
                        break;
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                set_ball_position(&ball, mouse_x, mouse_y);
            }
        }

        // Ball and paddle update logic
        update_ball(&ball, elapsed_time, LEFT_PADDLE_X, paddle_left_y, RIGHT_PADDLE_X, paddle_right_y, PADDLE_WIDTH, PADDLE_HEIGHT, &left_player_score, &right_player_score);

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render paddles and ball
        render_paddle(renderer, LEFT_PADDLE_X, paddle_left_y, PADDLE_WIDTH, PADDLE_HEIGHT);
        render_paddle(renderer, RIGHT_PADDLE_X, paddle_right_y, PADDLE_WIDTH, PADDLE_HEIGHT);
        render_ball(&ball);

        // Render score
        render_score(renderer, left_player_score, right_player_score);

        // Present the rendered frame
        SDL_RenderPresent(renderer);

        SDL_Delay(16);  // Limit to ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}