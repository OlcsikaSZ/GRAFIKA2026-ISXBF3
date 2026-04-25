#ifndef UTILS_H
#define UTILS_H

typedef struct vec3
{
    float x;
    float y;
    float z;
} vec3;

typedef struct Color
{
    float red;
    float green;
    float blue;
} Color;

typedef struct Material
{
    struct Color ambient;
    struct Color diffuse;
    struct Color specular;
    float shininess;
} Material;

// Convert an angle from degrees to radians.
double degree_to_radian(double degree);

// Draw bitmap text in screen space.
void draw_text_2d(int window_w, int window_h, int x_px, int y_px, const char* text);

// Draw a filled rectangle in screen space.
void draw_filled_rect_2d(int window_w, int window_h,
                         int x_px, int y_px, int w_px, int h_px,
                         float r, float g, float b, float a);

#endif /* UTILS_H */
