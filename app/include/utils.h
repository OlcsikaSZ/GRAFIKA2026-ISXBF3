#ifndef UTILS_H
#define UTILS_H

/**
 * GLSL-like three dimensional vector
 */
typedef struct vec3
{
    float x;
    float y;
    float z;
} vec3;

/**
 * Color with RGB components
 */
typedef struct Color
{
    float red;
    float green;
    float blue;
} Color;

/**
 * Material
 */
typedef struct Material
{
    struct Color ambient;
    struct Color diffuse;
    struct Color specular;
    float shininess;
} Material;

/**
 * Calculates radian from degree.
 */
double degree_to_radian(double degree);

// Minimal 8x8 bitmap text overlay (no extra deps like SDL_ttf).
// Coordinates are in pixels from the bottom-left corner.
void draw_text_2d(int window_w, int window_h, int x_px, int y_px, const char* text);

// Simple filled rectangle in screen space.
// Coordinates are in pixels from the bottom-left corner.
void draw_filled_rect_2d(int window_w, int window_h,
                         int x_px, int y_px, int w_px, int h_px,
                         float r, float g, float b, float a);

#endif /* UTILS_H */
