#include "matrix_stack.h"
#include <stdio.h>
#include <string.h>

void init_stack(MatrixStack* stack) {
    stack->top = -1; // Üres verem
}

void push_matrix(MatrixStack* stack, const float matrix[3][3]) {
    if (stack->top < STACK_CAPACITY - 1) {
        stack->top++;
        memcpy(stack->matrices[stack->top], matrix, sizeof(float) * 3 * 3);
    } else {
        printf("Stack overflow! Cannot push more matrices.\n");
    }
}

int pop_matrix(MatrixStack* stack, float matrix[3][3]) {
    if (stack->top >= 0) {
        memcpy(matrix, stack->matrices[stack->top], sizeof(float) * 3 * 3);
        stack->top--;
        return 1; // Sikeres művelet
    } else {
        printf("Stack underflow! Cannot pop matrix.\n");
        return 0; // Sikertelen művelet
    }
}

void print_top_matrix(const MatrixStack* stack) {
    if (stack->top >= 0) {
        printf("Top matrix:\n");
        print_matrix(stack->matrices[stack->top]);
    } else {
        printf("Stack is empty.\n");
    }
}