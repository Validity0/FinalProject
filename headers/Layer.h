#pragma once
#include "Matrix.h"

class Layer {
public:
    Matrix weights;
    Matrix biases;
    Matrix outputs;
    bool useTanh;  // Use tanh activation instead of sigmoid

    Layer(int input_size, int output_size, bool useTanh = false);
    Matrix forward(const Matrix& input) const;
};
