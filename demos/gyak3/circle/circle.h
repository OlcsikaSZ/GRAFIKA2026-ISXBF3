#ifndef CIRCLE_H
#define CIRCLE_H

// RGB színt reprezentáló struktúra
typedef struct Color {
    int r;  // Vörös (0-255)
    int g;  // Zöld (0-255)
    int b;  // Kék (0-255)
} Color;

/**
 * Data of a circle object in Descartes coordinate system
 */
typedef struct Circle {
    double x;       // Kör középpontjának x koordinátája
    double y;       // Kör középpontjának y koordinátája
    double radius;  // Kör sugara
    Color color;    // Kör színe
} Circle;

/**
 * Set the data of the circle, including color
 */
void set_circle_data(Circle* circle, double x, double y, double radius, int r, int g, int b);

#endif // CIRCLE_H