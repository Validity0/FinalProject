#pragma once
#include "Layer.h"
#include <vector>
#include <string>

// Training example: input-target pair
struct TrainingExample {
    Matrix input;
    Matrix target;

    TrainingExample() : input(1, 1), target(1, 1) {}
    TrainingExample(const Matrix& inp, const Matrix& tgt) : input(inp), target(tgt) {}
};

class NeuralNetwork {
public:
    std::vector<Layer> layers;

    NeuralNetwork(const std::vector<int>& layer_sizes);
    Matrix predict(const Matrix& input) const;
    void train(const Matrix& input, const Matrix& target, float learningRate);

    // Batch training: trains on all examples and returns average loss
    float trainBatch(const std::vector<TrainingExample>& batch, float learningRate);

    // Model persistence
    bool saveModel(const std::string& filename, bool verbose = true) const;
    bool loadModel(const std::string& filename, bool verbose = true);

    // Calculate loss for evaluation
    float calculateLoss(const std::vector<TrainingExample>& examples) const;
};
