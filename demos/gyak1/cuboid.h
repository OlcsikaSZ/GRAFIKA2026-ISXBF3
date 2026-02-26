//cuboid.h
#ifndef CUBOID_H
#define CUBOID_H

/**
 * Data of a cuboid object
 */
typedef struct Cuboid
{
    double length;
    double width;
    double height;
} Cuboid;

/**
 * Set the size of the cuboid
 */
void set_cuboid_size(Cuboid* cuboid, double length, double width, double height);

/**
 * Calculate the volume of the cuboid
 */
double calc_cuboid_volume(const Cuboid* cuboid);

/**
 * Calculate the surface area of the cuboid
 */
double calc_cuboid_surface(const Cuboid* cuboid);

/**
 * Check if the cuboid has a square face
 */
int has_square_face(const Cuboid* cuboid);

#endif // CUBOID_H