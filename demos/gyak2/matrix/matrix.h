//matrix.h
#ifndef MATRIX_H
#define MATRIX_H

/**
 * Initializes all elements of the matrix to zero.
 */
void init_zero_matrix(float matrix[3][3]);

/**
 * Initializes the matrix as an identity matrix.
 */
void init_identity_matrix(float matrix[3][3]);

/**
 * Print the elements of the matrix.
 */
void print_matrix(const float matrix[3][3]);

/**
 * Add matrices.
 */
void add_matrices(const float a[3][3], const float b[3][3], float c[3][3]);

/**
 * Multiply matrix by scalar.
 */
void multiply_matrix_by_scalar(float matrix[3][3], float scalar);

/**
 * Multiply matrices.
 */
void multiply_matrices(const float a[3][3], const float b[3][3], float result[3][3]);

/**
 * Transform a 2D point using a transformation matrix.
 */
void transform_point(const float matrix[3][3], float* x, float* y);

/**
 * Scale transformation.
 */
void scale(float matrix[3][3], float sx, float sy);

/**
 * Shift transformation.
 */
void shift(float matrix[3][3], float dx, float dy);

/**
 * Rotate transformation (angle in degrees).
 */
void rotate(float matrix[3][3], float angle);

#endif // MATRIX_H