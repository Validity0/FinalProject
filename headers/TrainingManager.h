#pragma once
#include "NeuralNetwork.h"
#include <string>
#include <vector>
#include <limits>

class TrainingManager {
private:
    NeuralNetwork* network;
    float bestLoss;
    int bestBatch;
    int totalBatches;
    bool lastSimWon = false;
    bool lastSimHit = false;
    bool hasWinningModel = false;  // Track if we've ever saved a winning model
    float bestWinLoss = std::numeric_limits<float>::max();  // Best loss among winning models
    const std::string BEST_MODEL_FILE = "best_model.nn";
    const std::string TRAINING_LOG_FILE = "training_log.txt";
    const int EXAMPLES_PER_BATCH = 32;  // Smaller batches = more iterations
    const float LEARNING_RATE = 0.1f;
    const int TRAINING_TIME_SECONDS = 600;  // 10 minutes
    const int DISPLAY_INTERVAL_BATCHES = 5000;

    // Generate training data for a batch
    std::vector<TrainingExample> generateBatchData(int numExamples);

    // Load or initialize training state
    void initializeTrainingState();

    // Save progress to log file
    void logBatchProgress(float batchLoss, float validationLoss, float improvement);

    // Load best model for batch training
    void loadBestModel();

    // Check and save new best model if validation loss improves
    void updateBestModel(float validationLoss);

    // Simulate game and calculate fitness score
    float simulateGameFitness(int simulationFrames = 1000);

public:
    TrainingManager(NeuralNetwork* nn);
    ~TrainingManager();

    // Run continuous training session
    void train();

    // Getters
    float getBestLoss() const { return bestLoss; }
    int getBestBatch() const { return bestBatch; }
    int getTotalBatches() const { return totalBatches; }
};
