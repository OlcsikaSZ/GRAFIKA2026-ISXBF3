//main.c
#include "matrix.h"
#include <stdio.h>

int main()
{
    float matrix[3][3];
    float x = 2.0f, y = 3.0f;

    printf("Inicializáló mátrix:\n");
    init_identity_matrix(matrix);
    print_matrix(matrix);

    printf("\nSkálázás (sx=2, sy=3):\n");
    scale(matrix, 2.0f, 3.0f);
    print_matrix(matrix);

    printf("\nElmozgatás (dx=4, dy=-2):\n");
    shift(matrix, 4.0f, -2.0f);
    print_matrix(matrix);

    printf("\nForgatás (45 fok):\n");
    rotate(matrix, 45.0f);
    print_matrix(matrix);

    printf("\nPont transzformáció előtt: (%.2f, %.2f)\n", x, y);
    transform_point(matrix, &x, &y);
    printf("Pont transzformáció után: (%.2f, %.2f)\n", x, y);

    return 0;
}