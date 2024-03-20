#include <stdio.h>
#include "Matrix.h"

#define M_PI (3.14159f)

#define  RAD2DEG (double)57.29577951


int main2() {
    // 测试矩阵加法
    Matrix a = { 3, 3, {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}} };
    Matrix b = { 3, 3, {{9, 8, 7}, {6, 5, 4}, {3, 2, 1}} };
    Matrix c = mat_add(a, b);
    printf("Matrix Addition:\n");
    print_matrix(a);
    print_matrix(b);
    print_matrix(c);

    // 测试矩阵乘法
    Matrix d = { 3, 2, {{1, 2}, {3, 4}, {5, 6}} };
    Matrix e = { 2, 3, {{7, 8, 9}, {10, 11, 12}} };
    Matrix f = mat_mul(d, e);
    printf("Matrix Multiplication:\n");
    print_matrix(d);
    print_matrix(e);
    print_matrix(f);

    return 0;
}

int main() {
    // double x = 1.0, y = 2.0, z = 1.0;
    // rotate_vector(&x, &y, &z, 0, 0, M_PI / 2);
    // 45,60,30
    // rotate_vector(&x, &y, &z, M_PI / 4, M_PI / 3, M_PI / 6);

    double x = 1.000, y = 0.002, z = -0.002;
    rotate_vector(&x, &y, &z, 178.05407/RAD2DEG, -85.19964/RAD2DEG, -178.54644/RAD2DEG);

    printf("Rotated vector: (%.2f, %.2f, %.2f)\n", x, y, z);
    return 0;
}