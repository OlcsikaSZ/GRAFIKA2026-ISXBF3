#include "help.h"
#include "texture.h"

#include <stdio.h>
#include <SDL2/SDL_opengl.h>

static int g_show_help = 0;
static GLuint g_help_tex = 0;

void toggle_help(void) {
    g_show_help = !g_show_help;
    if (g_show_help) {
        printf("\n=== MUSEUM CONTROLS (F1 to hide) ===\n");
        printf("WASD: move | Mouse: look\n");
        printf("B: human mode (walk + eye height)\n");
        printf("+ / - : light intensity\n");
        printf("F1: help\n");
        printf("ESC: quit\n");
        printf("===================================\n\n");
    }
}

int is_help_visible(void) {
    return g_show_help;
}

void draw_help_overlay(int w, int h) {
    if (!g_show_help) return;

    // Lazy-load help texture
    if (g_help_tex == 0) {
        // Use JPG to avoid libpng DLL issues on some systems.
        g_help_tex = load_texture("assets/textures/help.jpg");
    }

    // 2D overlay: orthographic projection, centered panel
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TRANSFORM_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Keep aspect ratio (~4:3). Occupy ~80% of the window.
    const float target_w = w * 0.82f;
    const float target_h = target_w * (768.0f / 1024.0f);
    float panel_w = target_w;
    float panel_h = target_h;
    if (panel_h > h * 0.82f) {
        panel_h = h * 0.82f;
        panel_w = panel_h * (1024.0f / 768.0f);
    }

    const float x0 = (w - panel_w) * 0.5f;
    const float y0 = (h - panel_h) * 0.5f;
    const float x1 = x0 + panel_w;
    const float y1 = y0 + panel_h;

    glBindTexture(GL_TEXTURE_2D, g_help_tex);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(x0, y0);
        glTexCoord2f(1, 0); glVertex2f(x1, y0);
        glTexCoord2f(1, 1); glVertex2f(x1, y1);
        glTexCoord2f(0, 1); glVertex2f(x0, y1);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glPopAttrib();
}
