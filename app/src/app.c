#include "app.h"
#include "help.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL_image.h>

static void reshape(App* app, GLsizei width, GLsizei height);

void init_app(App* app, int width, int height)
{
    int error_code;
    int inited_loaders;

    app->is_running = false;

    error_code = SDL_Init(SDL_INIT_EVERYTHING);
    if (error_code != 0) {
        printf("[ERROR] SDL initialization error: %s\n", SDL_GetError());
        return;
    }

    /* Request a stencil buffer for stencil-outline highlighting. */
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    app->window = SDL_CreateWindow(
        "Virtual Gallery – Interactive Museum Room",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL);
    if (app->window == NULL) {
        printf("[ERROR] Unable to create the application window!\n");
        return;
    }

    /*
     * NOTE:
     * SDL2_image is required for the assignment. However, initializing PNG
     * support on some Windows/MinGW setups can trigger a startup error
     * (inflateValidate / libpng16-16.dll) due to an outdated zlib/libpng pair
     * found on PATH. Since this project uses JPEG textures, we only initialize
     * JPG support here to avoid loading libpng at startup.
     */
    inited_loaders = IMG_Init(IMG_INIT_JPG);
    if ((inited_loaders & IMG_INIT_JPG) == 0) {
        printf("[ERROR] IMG init error: %s\n", IMG_GetError());
        return;
    }

    app->gl_context = SDL_GL_CreateContext(app->window);
    if (app->gl_context == NULL) {
        printf("[ERROR] Unable to create the OpenGL context!\n");
        return;
    }

    init_opengl();
    reshape(app, width, height);

    init_camera(&(app->camera));
    init_scene(&(app->scene));
    load_museum_scene(&(app->scene), "assets/config/scene.csv");

    app->uptime = (double)SDL_GetTicks() / 1000.0;

    app->is_running = true;
}

void init_opengl()
{
    glShadeModel(GL_SMOOTH);

    glEnable(GL_NORMALIZE);
    glEnable(GL_AUTO_NORMAL);

    // glClearColor(0.1, 0.1, 0.1, 1.0);
    glClearColor(1, 1, 1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_DEPTH_TEST);

    glClearDepth(1.0);

    // glEnable(GL_TEXTURE_2D);

    // glEnable(GL_COLOR_MATERIAL);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // glEnable(GL_FOG);
    // float fog_color[] = {1, 0.3, 0.8, 1};
    // glFogfv(GL_FOG_COLOR, fog_color);
    glDisable(GL_FOG);

    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

static void reshape(App* app, GLsizei width, GLsizei height)
{
    int x, y, w, h;
    double ratio;

    ratio = (double)width / height;
    if (ratio > VIEWPORT_RATIO) {
        w = (int)((double)height * VIEWPORT_RATIO);
        h = height;
        x = (width - w) / 2;
        y = 0;
    }
    else {
        w = width;
        h = (int)((double)width / VIEWPORT_RATIO);
        x = 0;
        y = (height - h) / 2;
    }

    glViewport(x, y, w, h);
    if (app) {
        app->viewport_x = x;
        app->viewport_y = y;
        app->viewport_w = w;
        app->viewport_h = h;
        app->window_w = width;
        app->window_h = height;
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    {
        // Szűkebb FOV 'ember módban' (természetesebb arányok), szélesebb 'fly' módban.
        const double human = app && app->camera.walk_bob_enabled;
        const double half_w = human ? 0.060 : 0.080;
        const double half_h = human ? 0.045 : 0.060;
        glFrustum(-half_w, half_w, -half_h, half_h, .1, 200.0);
    }
}

void handle_app_events(App* app)
{
    SDL_Event event;
    static bool is_mouse_down = false; // right button (mouse-look)
    static bool left_down = false;
    static int left_down_x = 0;
    static int left_down_y = 0;
    static int mouse_x = 0;
    static int mouse_y = 0;
    int x;
    int y;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE:
                app->is_running = false;
                break;
            case SDL_SCANCODE_W:
                set_camera_speed(&(app->camera), 1);
                break;
            case SDL_SCANCODE_S:
                set_camera_speed(&(app->camera), -1);
                break;
            case SDL_SCANCODE_A:
                set_camera_side_speed(&(app->camera), 1);
                break;
            case SDL_SCANCODE_D:
                set_camera_side_speed(&(app->camera), -1);
                break;
            case SDL_SCANCODE_Q:
                // "Séta" módban ne lehessen repülni
                if (!app->camera.walk_bob_enabled) {
                    set_camera_vertical_speed(&(app->camera), 1);
                }
                break;
            case SDL_SCANCODE_E:
                // "Séta" módban ne lehessen repülni
                if (!app->camera.walk_bob_enabled) {
                    set_camera_vertical_speed(&(app->camera), -1);
                }
                break;
            case SDL_SCANCODE_F1:
                toggle_help();
                break;
            case SDL_SCANCODE_R:
                // Statue forgás indítás/megállítás
                toggle_animation(&(app->scene));
                break;
            case SDL_SCANCODE_H:
                // Shadows on/off
                toggle_shadows(&(app->scene));
                break;
            case SDL_SCANCODE_B:
                // Walking head-bob (járás érzet)
                toggle_walk_bob(&(app->camera));
                // kapcsoláskor biztosan állítsuk le a vertikális mozgást
                set_camera_vertical_speed(&(app->camera), 0);
                // FOV frissítés (ember mód szűkebb)
                reshape(app, app->window_w, app->window_h);
                break;
            case SDL_SCANCODE_KP_PLUS:
            case SDL_SCANCODE_EQUALS: /* + on main keyboard (shift+=) */
                change_light(&(app->scene), 0.1f);
                break;
            case SDL_SCANCODE_KP_MINUS:
            case SDL_SCANCODE_MINUS:  /* - on main keyboard */
                change_light(&(app->scene), -0.1f);
                break;
            default:
                break;
            }
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_W:
            case SDL_SCANCODE_S:
                set_camera_speed(&(app->camera), 0);
                break;
            case SDL_SCANCODE_A:
            case SDL_SCANCODE_D:
                set_camera_side_speed(&(app->camera), 0);
                break;
            case SDL_SCANCODE_Q:
            case SDL_SCANCODE_E:
                set_camera_vertical_speed(&(app->camera), 0);
                break;
            default:
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_RIGHT) {
                is_mouse_down = true;
            }
            if (event.button.button == SDL_BUTTON_LEFT) {
                left_down = true;
                left_down_x = event.button.x;
                left_down_y = event.button.y;
            }
            break;
        case SDL_MOUSEMOTION:
            SDL_GetMouseState(&x, &y);
            if (is_mouse_down) {
                rotate_camera(&(app->camera), mouse_x - x, mouse_y - y);
            }
            mouse_x = x;
            mouse_y = y;
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_RIGHT) {
                is_mouse_down = false;
            }
            if (event.button.button == SDL_BUTTON_LEFT && left_down) {
                left_down = false;
                const int dx = abs(event.button.x - left_down_x);
                const int dy = abs(event.button.y - left_down_y);
                if (dx + dy <= 3) {
                    const int idx = pick_entity(
                        &app->scene, &app->camera,
                        event.button.x, event.button.y,
                        app->viewport_x, app->viewport_y, app->viewport_w, app->viewport_h);

                    // Convenience: clicking the statue toggles animation
                    if (idx >= 0 && strcmp(app->scene.entities[idx].type, "statue") == 0) {
                        toggle_animation(&(app->scene));
                    }

                    // Also reflect selection in the window title (handy + obvious for demo).
                    if (idx >= 0) {
                        char title[128];
                        snprintf(title, sizeof(title), "Virtual Gallery – Interactive Museum Room | Selected: %s (#%d)",
                                 app->scene.entities[idx].type, idx);
                        SDL_SetWindowTitle(app->window, title);
                    } else {
                        SDL_SetWindowTitle(app->window, "Virtual Gallery – Interactive Museum Room");
                    }
                }
            }
            break;
        case SDL_QUIT:
            app->is_running = false;
            break;
        default:
            break;
        }
    }
}

void update_app(App* app)
{
    double current_time;
    double elapsed_time;

    current_time = (double)SDL_GetTicks() / 1000;
    elapsed_time = current_time - app->uptime;
    app->uptime = current_time;

    update_camera(&(app->camera), elapsed_time);
    update_scene(&(app->scene), elapsed_time);
}

void render_app(App* app)
{
    int w = 0, h = 0;
    SDL_GetWindowSize(app->window, &w, &h);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();
    set_view(&(app->camera));
    render_scene(&(app->scene));
    glPopMatrix();

    if (app->camera.is_preview_visible) {
        show_texture_preview();
    }

    if (is_help_visible()) {
        int w = 0, h = 0;
        SDL_GetWindowSize(app->window, &w, &h);
        draw_help_overlay(w, h);
    }

    // Picking info panel (bottom-left). Make it readable + a bit more helpful.
    {
        int ww = 0, hh = 0;
        SDL_GetWindowSize(app->window, &ww, &hh);

        const int panel_x = 12;
        const int panel_y = hh - 72;   // top-left style
        const int panel_w = 340;
        const int panel_h = 58;

        draw_filled_rect_2d(ww, hh, panel_x, panel_y, panel_w, panel_h, 0.f, 0.f, 0.f, 0.45f);

        if (app->scene.selected_entity >= 0 && app->scene.selected_entity < app->scene.entity_count) {
            const Entity* e = &app->scene.entities[app->scene.selected_entity];
            char buf[256];
            snprintf(buf, sizeof(buf), "Selected: %s (#%d)\nLMB pick | T anim (statue)",
                     e->type, app->scene.selected_entity);
            draw_text_2d(ww, hh, panel_x + 10, panel_y + 10, buf);
        } else {
            draw_text_2d(ww, hh, panel_x + 10, panel_y + 10, "Click to pick\nObjects will highlight");
        }
    }

    SDL_GL_SwapWindow(app->window);
}

void destroy_app(App* app)
{
    if (app->gl_context != NULL) {
        SDL_GL_DeleteContext(app->gl_context);
    }

    if (app->window != NULL) {
        SDL_DestroyWindow(app->window);
    }

    SDL_Quit();
}
