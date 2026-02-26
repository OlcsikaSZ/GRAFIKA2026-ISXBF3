#include "cuboid.h"
#include <stdio.h>

int main(int argc, char* argv[])
{
    Cuboid cuboid;
    double volume, surface;
    
    set_cuboid_size(&cuboid, 3, 4, 4);
    
    volume = calc_cuboid_volume(&cuboid);
    surface = calc_cuboid_surface(&cuboid);
    
    printf("Cuboid volume: %.2f\n", volume);
    printf("Cuboid surface: %.2f\n", surface);
    
    if (has_square_face(&cuboid)) {
        printf("The cuboid has at least one square face.\n");
    } else {
        printf("The cuboid has no square faces.\n");
    }
    
    return 0;
}