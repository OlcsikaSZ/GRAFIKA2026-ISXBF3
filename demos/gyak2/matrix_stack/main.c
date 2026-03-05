#include "matrix.h"
#include "matrix_stack.h"
#include <stdio.h>

int main() {
    MatrixStack stack;
    float matrix[3][3];

    // Inicializáljuk a veremet és a kezdő mátrixot
    init_stack(&stack);
    init_identity_matrix(matrix);

    // Nyomtatjuk az alapállapotot
    printf("Starting matrix:\n");
    print_matrix(matrix);

    // 1. Módosítás: Skálázás
    push_matrix(&stack, matrix); // Mentjük az előző állapotot
    scale(matrix, 2.0f, 3.0f);
    printf("\nAfter scaling:\n");
    print_matrix(matrix);

    // 2. Módosítás: Eltolás
    push_matrix(&stack, matrix); // Mentjük az előző állapotot
    shift(matrix, 5.0f, -3.0f);
    printf("\nAfter shifting:\n");
    print_matrix(matrix);

    // 3. Módosítás: Forgatás
    push_matrix(&stack, matrix); // Mentjük az előző állapotot
    rotate(matrix, 30.0f);
    printf("\nAfter rotation:\n");
    print_matrix(matrix);

    // Visszavonások:
    printf("\nUndo last transformation (pop_matrix):\n");
    if (pop_matrix(&stack, matrix)) print_matrix(matrix);

    printf("\nUndo another transformation (pop_matrix):\n");
    if (pop_matrix(&stack, matrix)) print_matrix(matrix);

    printf("\nUndo last transformation (pop_matrix):\n");
    if (pop_matrix(&stack, matrix)) print_matrix(matrix);

    return 0;
}