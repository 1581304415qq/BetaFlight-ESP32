#include "Matrix.h"
#include "math.h"
#include <stdio.h>

// 打印矩阵
void print_matrix(Matrix m) {
    for (int i = 0; i < m.rows; i++) {
        for (int j = 0; j < m.cols; j++) {
            printf("%.2lf ", m.data[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}



// 矩阵加法
Matrix mat_add(Matrix a, Matrix b) {
    Matrix c;
    // 检查两个矩阵维数是否相同
    if (a.rows != b.rows || a.cols != b.cols) {
        c.rows = c.cols = 0;
        return c;
    }
    c.rows = a.rows;
    c.cols = a.cols;
    for (int i = 0; i < a.rows; i++) {
        for (int j = 0; j < a.cols; j++) {
            c.data[i][j] = a.data[i][j] + b.data[i][j];
        }
    }
    return c;
}

// 矩阵乘法
Matrix mat_mul(Matrix a, Matrix b) {
    Matrix c;
    if (a.cols != b.rows) {
        c.rows = c.cols = 0;
        return c;
    }
    c.rows = a.rows;
    c.cols = b.cols;
    for (int i = 0; i < c.rows; i++) {
        for (int j = 0; j < c.cols; j++) {
            c.data[i][j] = 0;
            for (int k = 0; k < a.cols; k++) {
                c.data[i][j] += a.data[i][k] * b.data[k][j];
            }
        }
    }
    return c;
}

// 绕 X 轴旋转矩阵
Matrix rotate_x(double theta) {
    Matrix rx = { 3, 3,
                {{1, 0, 0},
                {0, cos(theta), -sin(theta)},
                {0, sin(theta), cos(theta)}} };
    return rx;
}

// 绕 Y 轴旋转矩阵 
Matrix rotate_y(double theta) {
    Matrix ry = { 3, 3,
                {{cos(theta), 0, sin(theta)},
                {0, 1, 0},
                {-sin(theta), 0, cos(theta)}} };
    return ry;
}

// 绕 Z 轴旋转矩阵
Matrix rotate_z(double theta) {
    Matrix rz = { 3, 3,
                {{cos(theta), -sin(theta), 0},
                {sin(theta), cos(theta), 0},
                {0, 0, 1}} };
    return rz;
}

// 3D 向量旋转
void rotate_vector(double* x, double* y, double* z, double theta_x, double theta_y, double theta_z) {
    Matrix rx = rotate_x(theta_x);
    Matrix ry = rotate_y(theta_y);
    Matrix rz = rotate_z(theta_z);
    // print_matrix(rx);
    // print_matrix(ry);
    // print_matrix(rz);
    Matrix r = mat_mul(mat_mul(rz, ry), rx);
    // print_matrix(r);
    Matrix v = { 3, 1, {{*x}, {*y}, {*z}} };
    // print_matrix(v);
    Matrix res = mat_mul(r, v);
    // print_matrix(res);
    *x = res.data[0][0];
    *y = res.data[1][0];
    *z = res.data[2][0];
}