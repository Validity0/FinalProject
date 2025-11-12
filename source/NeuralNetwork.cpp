#include "NeuralNetwork.h"
#include <cmath>

// Derivative of sigmoid
static float dsigmoid(float y) {
    return y * (1.0f - y);
}

NeuralNetwork::NeuralNetwork(const std::vector<int>& sizes) {
    for (size_t i = 1; i < sizes.size(); ++i)
        layers.emplace_back(sizes[i - 1], sizes[i]);
}

Matrix NeuralNetwork::predict(const Matrix& input) {
    Matrix out = input;
    for (auto& layer : layers)
        out = layer.forward(out);
    return out;
}

void NeuralNetwork::train(const Matrix& input, const Matrix& target, float learningRate) {
    // Forward pass
    Matrix hiddenOutput = layers[0].forward(input);
    Matrix finalOutput = layers[1].forward(hiddenOutput);

    // Error at output
    Matrix outputError(target.rows, target.cols);
    for (int i = 0; i < target.rows; ++i)
        for (int j = 0; j < target.cols; ++j)
            outputError.data[i][j] = target.data[i][j] - finalOutput.data[i][j];

    // Gradient for output layer
    Matrix outputGradient = finalOutput.apply(dsigmoid);
    for (int i = 0; i < outputGradient.rows; ++i)
        for (int j = 0; j < outputGradient.cols; ++j)
            outputGradient.data[i][j] *= outputError.data[i][j] * learningRate;

    // Delta weights for output layer
    Matrix hiddenT = hiddenOutput.transpose();
    Matrix outputDelta = hiddenT.dot(outputGradient);

    // Update output layer weights and biases
    for (int i = 0; i < layers[1].weights.rows; ++i)
        for (int j = 0; j < layers[1].weights.cols; ++j)
            layers[1].weights.data[i][j] += outputDelta.data[i][j];

    for (int j = 0; j < layers[1].biases.cols; ++j)
        layers[1].biases.data[0][j] += outputGradient.data[0][j];

    // Error at hidden layer
    Matrix outputWeightsT = layers[1].weights.transpose();
    Matrix hiddenError = outputGradient.dot(outputWeightsT);

    // Gradient for hidden layer
    Matrix hiddenGradient = hiddenOutput.apply(dsigmoid);
    for (int i = 0; i < hiddenGradient.rows; ++i)
        for (int j = 0; j < hiddenGradient.cols; ++j)
            hiddenGradient.data[i][j] *= hiddenError.data[i][j] * learningRate;

    // Delta weights for hidden layer
    Matrix inputT = input.transpose();
    Matrix hiddenDelta = inputT.dot(hiddenGradient);

    // Update hidden layer weights and biases
    for (int i = 0; i < layers[0].weights.rows; ++i)
        for (int j = 0; j < layers[0].weights.cols; ++j)
            layers[0].weights.data[i][j] += hiddenDelta.data[i][j];

    for (int j = 0; j < layers[0].biases.cols; ++j)
        layers[0].biases.data[0][j] += hiddenGradient.data[0][j];
}

