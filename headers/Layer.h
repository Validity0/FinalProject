#pragma once
#include "Matrix.h"

class Layer {
public:
    Matrix weights;
    Matrix biases;
    Matrix outputs;

    Layer(int input_size, int output_size);
    Matrix forward(const Matrix& input);
};
