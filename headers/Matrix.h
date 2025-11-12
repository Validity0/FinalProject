#pragma once
#include <vector>

class Matrix {
public:
    int rows, cols;
    std::vector<std::vector<float>> data;

    Matrix(int rows, int cols);
    void randomize(float min = -1.0f, float max = 1.0f);
    Matrix dot(const Matrix& other) const;
    Matrix add(const Matrix& other) const;
    Matrix apply(float (*func)(float)) const;
    Matrix transpose() const;
    void print() const;
};