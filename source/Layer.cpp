#include "Layer.h"
#include <cmath>

static float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

static float tanhActivation(float x) {
    return std::tanh(x);
}

Layer::Layer(int input_size, int output_size, bool useTanh)
    : weights(input_size, output_size), biases(1, output_size), outputs(1, output_size), useTanh(useTanh) {
    weights.randomize(-0.5f, 0.5f);
    biases.randomize(-0.5f, 0.5f);
}

Matrix Layer::forward(const Matrix& input) const {
    if (useTanh) {
        return input.dot(weights).add(biases).apply(tanhActivation);
    }
    return input.dot(weights).add(biases).apply(sigmoid);
}
