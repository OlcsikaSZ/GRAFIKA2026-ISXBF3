#include "circle.h"

/**
 * Set the data of the circle, including position, radius, and color
 */
void set_circle_data(Circle* circle, double x, double y, double radius, int r, int g, int b) {
    circle->x = x;
    circle->y = y;
    circle->radius = radius;
    circle->color.r = r;
    circle->color.g = g;
    circle->color.b = b;
}