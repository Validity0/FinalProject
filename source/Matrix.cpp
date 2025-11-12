#include "Matrix.h"
#include <iostream>
#include <random>

// Constructor: creates a matrix with given rows and columns, filled with 0.0
Matrix::Matrix(int r, int c) : rows(r), cols(c), data(r, std::vector<float>(c, 0.0f)) {}

// Fill the matrix with random values between min and max
void Matrix::randomize(float min, float max) {
    std::mt19937 rng(42); // fixed seed for reproducibility
    std::uniform_real_distribution<float> dist(min, max);
    for (auto& row : data)
        for (auto& val : row)
            val = dist(rng); // assign random value to each cell
}

// Matrix multiplication: this * other
Matrix Matrix::dot(const Matrix& other) const {
    Matrix result(rows, other.cols); // result size: rows of this × cols of other
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < other.cols; ++j)
            for (int k = 0; k < cols; ++k)
                result.data[i][j] += data[i][k] * other.data[k][j]; // sum of products
    return result;
}

// Element-wise addition: this + other
Matrix Matrix::add(const Matrix& other) const {
    Matrix result(rows, cols);
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            result.data[i][j] = data[i][j] + other.data[i][j]; // add each element
    return result;
}

// Apply a function to every element (e.g., sigmoid)
Matrix Matrix::apply(float (*func)(float)) const {
    Matrix result(rows, cols);
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            result.data[i][j] = func(data[i][j]); // apply function to each cell
    return result;
}

// Transpose the matrix: flip rows and columns
Matrix Matrix::transpose() const {
    Matrix result(cols, rows); // new shape: cols × rows
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            result.data[j][i] = data[i][j]; // swap row and column
    return result;
}

// Print the matrix to console (for debugging)
void Matrix::print() const {
    for (const auto& row : data) {
        for (float val : row)
            std::cout << val << " "; // print each value in row
        std::cout << "\n"; // new line after each row
    }
}
