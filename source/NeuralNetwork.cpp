#include "NeuralNetwork.h"
#include <cmath>
#include <fstream>
#include <iostream>

// Derivative of sigmoid
static float dsigmoid(float y) {
    return y * (1.0f - y);
}

NeuralNetwork::NeuralNetwork(const std::vector<int>& sizes) {
    for (size_t i = 1; i < sizes.size(); ++i)
        layers.emplace_back(sizes[i - 1], sizes[i]);
}

Matrix NeuralNetwork::predict(const Matrix& input) const {
    Matrix out = input;
    for (const auto& layer : layers)
        out = layer.forward(out);
    return out;
}

void NeuralNetwork::train(const Matrix& input, const Matrix& target, float learningRate) {
    // Forward pass through all layers and store outputs
    std::vector<Matrix> layerOutputs;
    layerOutputs.push_back(input);

    Matrix currentOutput = input;
    for (auto& layer : layers) {
        currentOutput = layer.forward(currentOutput);
        layerOutputs.push_back(currentOutput);
    }

    // Backward pass through all layers
    int numLayers = layers.size();

    // Error at output layer
    Matrix error(target.rows, target.cols);
    for (int i = 0; i < target.rows; ++i)
        for (int j = 0; j < target.cols; ++j)
            error.data[i][j] = target.data[i][j] - layerOutputs[numLayers].data[i][j];

    // Backpropagate through layers in reverse
    for (int l = numLayers - 1; l >= 0; --l) {
        // Gradient for current layer
        Matrix gradient = layerOutputs[l + 1].apply(dsigmoid);
        for (int i = 0; i < gradient.rows; ++i)
            for (int j = 0; j < gradient.cols; ++j)
                gradient.data[i][j] *= error.data[i][j] * learningRate;

        // Delta weights
        Matrix inputT = layerOutputs[l].transpose();
        Matrix delta = inputT.dot(gradient);

        // Update weights and biases
        for (int i = 0; i < layers[l].weights.rows; ++i)
            for (int j = 0; j < layers[l].weights.cols; ++j)
                layers[l].weights.data[i][j] += delta.data[i][j];

        for (int j = 0; j < layers[l].biases.cols; ++j)
            layers[l].biases.data[0][j] += gradient.data[0][j];

        // Compute error for previous layer if not input layer
        if (l > 0) {
            Matrix weightsT = layers[l].weights.transpose();
            error = gradient.dot(weightsT);
        }
    }
}

float NeuralNetwork::trainBatch(const std::vector<TrainingExample>& batch, float learningRate) {
    float totalLoss = 0.0f;

    // Train on each example in the batch
    for (const auto& example : batch) {
        train(example.input, example.target, learningRate);

        // Calculate loss for this example (MSE)
        Matrix prediction = predict(example.input);
        for (int i = 0; i < example.target.rows; ++i) {
            for (int j = 0; j < example.target.cols; ++j) {
                float error = example.target.data[i][j] - prediction.data[i][j];
                totalLoss += error * error;
            }
        }
    }

    // Return average loss
    return totalLoss / batch.size();
}

float NeuralNetwork::calculateLoss(const std::vector<TrainingExample>& examples) const {
    float totalLoss = 0.0f;
    int totalOutputs = 0;

    for (const auto& example : examples) {
        Matrix prediction = predict(example.input);
        for (int i = 0; i < example.target.rows; ++i) {
            for (int j = 0; j < example.target.cols; ++j) {
                float error = example.target.data[i][j] - prediction.data[i][j];
                totalLoss += error * error;
                totalOutputs++;
            }
        }
    }

    return totalOutputs > 0 ? totalLoss / totalOutputs : 0.0f;
}

bool NeuralNetwork::saveModel(const std::string& filename, bool verbose) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        if (verbose) {
            std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
        }
        return false;
    }

    // Save number of layers
    int numLayers = layers.size();
    file.write(reinterpret_cast<char*>(&numLayers), sizeof(numLayers));

    // Save each layer's weights and biases
    for (const auto& layer : layers) {
        // Save weights
        int weightRows = layer.weights.rows;
        int weightCols = layer.weights.cols;
        file.write(reinterpret_cast<char*>(&weightRows), sizeof(weightRows));
        file.write(reinterpret_cast<char*>(&weightCols), sizeof(weightCols));

        for (int i = 0; i < weightRows; ++i) {
            for (int j = 0; j < weightCols; ++j) {
                float val = layer.weights.data[i][j];
                file.write(reinterpret_cast<char*>(&val), sizeof(val));
            }
        }

        // Save biases
        int biasRows = layer.biases.rows;
        int biasCols = layer.biases.cols;
        file.write(reinterpret_cast<char*>(&biasRows), sizeof(biasRows));
        file.write(reinterpret_cast<char*>(&biasCols), sizeof(biasCols));

        for (int i = 0; i < biasRows; ++i) {
            for (int j = 0; j < biasCols; ++j) {
                float val = layer.biases.data[i][j];
                file.write(reinterpret_cast<char*>(&val), sizeof(val));
            }
        }
    }

    file.close();
    if (verbose) {
        std::cout << "Model saved to: " << filename << std::endl;
    }
    return true;
}

bool NeuralNetwork::loadModel(const std::string& filename, bool verbose) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        if (verbose) {
            std::cerr << "Error: Could not open file for reading: " << filename << std::endl;
        }
        return false;
    }

    // Load number of layers
    int numLayers;
    file.read(reinterpret_cast<char*>(&numLayers), sizeof(numLayers));

    if (numLayers != layers.size()) {
        if (verbose) {
            std::cerr << "Error: Model file has " << numLayers << " layers but network has "
                      << layers.size() << " layers" << std::endl;
        }
        file.close();
        return false;
    }

    // Load each layer's weights and biases
    for (auto& layer : layers) {
        // Load weights
        int weightRows, weightCols;
        file.read(reinterpret_cast<char*>(&weightRows), sizeof(weightRows));
        file.read(reinterpret_cast<char*>(&weightCols), sizeof(weightCols));

        if (weightRows != layer.weights.rows || weightCols != layer.weights.cols) {
            if (verbose) {
                std::cerr << "Error: Weight dimensions mismatch" << std::endl;
            }
            file.close();
            return false;
        }

        for (int i = 0; i < weightRows; ++i) {
            for (int j = 0; j < weightCols; ++j) {
                float val;
                file.read(reinterpret_cast<char*>(&val), sizeof(val));
                layer.weights.data[i][j] = val;
            }
        }

        // Load biases
        int biasRows, biasCols;
        file.read(reinterpret_cast<char*>(&biasRows), sizeof(biasRows));
        file.read(reinterpret_cast<char*>(&biasCols), sizeof(biasCols));

        if (biasRows != layer.biases.rows || biasCols != layer.biases.cols) {
            if (verbose) {
                std::cerr << "Error: Bias dimensions mismatch" << std::endl;
            }
            file.close();
            return false;
        }

        for (int i = 0; i < biasRows; ++i) {
            for (int j = 0; j < biasCols; ++j) {
                float val;
                file.read(reinterpret_cast<char*>(&val), sizeof(val));
                layer.biases.data[i][j] = val;
            }
        }
    }

    file.close();
    if (verbose) {
        std::cout << "Model loaded from: " << filename << std::endl;
    }
    return true;
}

