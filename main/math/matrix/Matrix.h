#pragma once
#define MAX_SIZE 10

typedef struct {
    int rows, cols;
    double data[MAX_SIZE][MAX_SIZE];
} Matrix;

void print_matrix(Matrix m);
Matrix mat_add(Matrix a, Matrix b);
Matrix mat_mul(Matrix a, Matrix b);
Matrix rotate_x(double theta);
Matrix rotate_y(double theta);
Matrix rotate_z(double theta);
void rotate_vector(double *x, double *y, double *z, double theta_x, double theta_y, double theta_z);
