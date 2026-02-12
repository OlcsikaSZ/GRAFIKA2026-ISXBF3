#include "help.h"
#include <stdio.h>

static int g_show_help = 0;

void toggle_help(void) {
    g_show_help = !g_show_help;
    if (g_show_help) {
        printf("\n=== MUSEUM CONTROLS (F1 to hide) ===\n");
        printf("WASD: move | Mouse: look\n");
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
    (void)w; (void)h;
    // MVP-ben elég a printf-es help. Később ide jön a text render (me-courses text example).
}
