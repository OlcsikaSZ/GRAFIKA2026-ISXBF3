//matrix.c
#include "matrix.h"
#include <stdio.h>
#include <math.h>

#define DEG_TO_RAD(angle) ((angle) * M_PI / 180.0f)

void init_zero_matrix(float matrix[3][3])
{
    int i, j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            matrix[i][j] = 0.0f;
        }
    }
}

void init_identity_matrix(float matrix[3][3])
{
    int i, j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            matrix[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

void print_matrix(const float matrix[3][3])
{
    int i, j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            printf("%6.2f ", matrix[i][j]);
        }
        printf("\n");
    }
}

void add_matrices(const float a[3][3], const float b[3][3], float c[3][3])
{
    int i, j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            c[i][j] = a[i][j] + b[i][j];
        }
    }
}

void multiply_matrix_by_scalar(float matrix[3][3], float scalar)
{
    int i, j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            matrix[i][j] *= scalar;
        }
    }
}

void multiply_matrices(const float a[3][3], const float b[3][3], float result[3][3])
{
    int i, j, k;
    float temp[3][3] = {0};

    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            for (k = 0; k < 3; ++k) {
                temp[i][j] += a[i][k] * b[k][j];
            }
        }
    }

    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            result[i][j] = temp[i][j];
        }
    }
}

void transform_point(const float matrix[3][3], float* x, float* y)
{
    float new_x = matrix[0][0] * (*x) + matrix[0][1] * (*y) + matrix[0][2];
    float new_y = matrix[1][0] * (*x) + matrix[1][1] * (*y) + matrix[1][2];
    
    *x = new_x;
    *y = new_y;
}

void scale(float matrix[3][3], float sx, float sy)
{
    float scale_matrix[3][3] = {
        {sx, 0,  0},
        {0,  sy, 0},
        {0,  0,  1}
    };
    multiply_matrices(matrix, scale_matrix, matrix);
}

void shift(float matrix[3][3], float dx, float dy)
{
    float shift_matrix[3][3] = {
        {1, 0, dx},
        {0, 1, dy},
        {0, 0, 1}
    };
    multiply_matrices(matrix, shift_matrix, matrix);
}

void rotate(float matrix[3][3], float angle)
{
    float rad = DEG_TO_RAD(angle);
    float cos_a = cosf(rad);
    float sin_a = sinf(rad);

    float rotate_matrix[3][3] = {
        {cos_a, -sin_a, 0},
        {sin_a, cos_a,  0},
        {0,     0,      1}
    };
    multiply_matrices(matrix, rotate_matrix, matrix);
}