#ifndef APP_H
#define APP_H

#include "camera.h"
#include "scene.h"

#include <SDL2/SDL.h>
#include <stdbool.h>

#define VIEWPORT_RATIO (4.0 / 3.0)
#define VIEWPORT_ASPECT 50.0

typedef struct App
{
    SDL_Window* window;
    SDL_GLContext gl_context;
    bool is_running;
    double uptime;
    Camera camera;
    Scene scene;

    // Store the letterboxed viewport for rendering and picking.
    int viewport_x;
    int viewport_y;
    int viewport_w;
    int viewport_h;

    int window_w;
    int window_h;

    // Store the windowed state when toggling fullscreen.
    bool is_fullscreen;
    int windowed_x;
    int windowed_y;
    int windowed_w;
    int windowed_h;
} App;

// Initialize SDL, OpenGL, the camera, and the scene.
void init_app(App* app, int width, int height);

// Set the OpenGL state used by the application.
void init_opengl();

// Process keyboard, mouse, and window events.
void handle_app_events(App* app);

// Advance application state using elapsed time.
void update_app(App* app);

// Render the full frame and the UI overlays.
void render_app(App* app);

// Release all SDL, OpenGL, and scene resources.
void destroy_app(App* app);

void show_texture_preview(void);

#endif /* APP_H */
