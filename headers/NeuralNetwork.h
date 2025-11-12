#pragma once
#include "Layer.h"
#include <vector>

class NeuralNetwork {
public:
    std::vector<Layer> layers;

    NeuralNetwork(const std::vector<int>& layer_sizes);
    Matrix predict(const Matrix& input);
    void train(const Matrix& input, const Matrix& target, float learningRate);
};
