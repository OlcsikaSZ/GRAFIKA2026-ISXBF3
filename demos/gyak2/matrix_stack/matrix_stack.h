#ifndef MATRIX_STACK_H
#define MATRIX_STACK_H

#include "matrix.h"

#define STACK_CAPACITY 10  // Kezdő kapacitás, bővíthető igény szerint

typedef struct {
    float matrices[STACK_CAPACITY][3][3];
    int top;
} MatrixStack;

/**
 * Initialize the matrix stack.
 */
void init_stack(MatrixStack* stack);

/**
 * Push a matrix onto the stack.
 */
void push_matrix(MatrixStack* stack, const float matrix[3][3]);

/**
 * Pop a matrix from the stack.
 */
int pop_matrix(MatrixStack* stack, float matrix[3][3]);

/**
 * Print the current top matrix of the stack.
 */
void print_top_matrix(const MatrixStack* stack);

#endif // MATRIX_STACK_H