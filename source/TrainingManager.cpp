#include "TrainingManager.h"
#include "SpaceShip.h"
#include "GameSettings.h"
#include "GameLogic.h"
#include <iostream>
#include <fstream>
#include <random>
#include <iomanip>
#include <limits>
#include <chrono>
#include <cmath>
#include <conio.h>

TrainingManager::TrainingManager(NeuralNetwork* nn)
    : network(nn), bestLoss(std::numeric_limits<float>::max()), bestBatch(0), totalBatches(0)
{
}

TrainingManager::~TrainingManager()
{
}

std::vector<TrainingExample> TrainingManager::generateBatchData(int numExamples)
{
    std::vector<TrainingExample> examples;
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int i = 0; i < numExamples; ++i) {
        // Generate random 12-input game state matching simulation and game mode
        float shipX = dist(rng);
        float shipY = dist(rng);
        float shipVelX = (dist(rng) * 2.0f - 1.0f);
        float shipVelY = (dist(rng) * 2.0f - 1.0f);
        float shipRotation = dist(rng);

        // Station relative info
        float stationDx = (STATION_X / 400.0f - shipX);
        float stationDy = (STATION_Y / 300.0f - shipY);
        float stationDist = std::sqrt(stationDx * stationDx + stationDy * stationDy);
        float stationAngle = std::atan2(stationDy, stationDx) / 3.14159f;

        // Bullet info (random closest bullet)
        float closestBulletDist = dist(rng) * 2.0f;
        float closestBulletAngle = (dist(rng) * 2.0f - 1.0f);
        float closestBulletVelX = (dist(rng) * 2.0f - 1.0f);
        float closestBulletVelY = (dist(rng) * 2.0f - 1.0f);
        float numBullets = dist(rng);

        // Create 12-input example
        TrainingExample example;
        example.input = Matrix(1, 12);
        example.input.data[0] = {
            shipX,                      // 0: Ship X
            shipY,                      // 1: Ship Y
            shipVelX,                   // 2: Ship velocity X
            shipVelY,                   // 3: Ship velocity Y
            shipRotation,               // 4: Ship rotation
            stationDist,                // 5: Station distance
            stationAngle,               // 6: Station angle
            closestBulletDist,          // 7: Closest bullet distance
            closestBulletAngle,         // 8: Closest bullet angle
            closestBulletVelX,          // 9: Closest bullet vel X
            closestBulletVelY,          // 10: Closest bullet vel Y
            numBullets                  // 11: Number of bullets
        };

        // Target 4 outputs: thrust, strafe, rotation, brake
        // IMPORTANT: Sigmoid outputs 0-1, so targets must be 0-1
        // After transformation in simulation: (output - 0.5) * 2 gives -1 to +1
        // So: target 0.0 = -1 (full left), 0.5 = 0 (neutral), 1.0 = +1 (full right)

        float targetThrust = 0.5f;  // Default moderate thrust
        float strafeRaw = 0.0f;
        float rotationRaw = 0.0f;
        float targetBrake = 0.0f;

        // PRIORITY 1: Bullet avoidance when bullet is close
        if (closestBulletDist < 0.4f) {
            // Bullet is dangerous! Strafe perpendicular to bullet direction
            float bulletAngleRad = closestBulletAngle * 3.14159f;
            float perpAngle = bulletAngleRad + 1.5708f;  // Add 90 degrees

            // Strafe away from bullet path
            strafeRaw = std::sin(perpAngle);

            // Rotate away from bullet - ensure full range coverage
            rotationRaw = -closestBulletAngle;  // This gives -1 to +1 range

            // Thrust to escape
            targetThrust = 0.8f;
            targetBrake = 0.0f;
        }
        // PRIORITY 2: Move toward station when safe
        else {
            targetThrust = (stationDist > 0.3f) ? 0.7f : 0.3f;
            strafeRaw = std::sin(stationAngle * 3.14159f) * 0.5f;

            // stationAngle is already -1 to +1 (divided by pi earlier)
            // Use it directly for rotation - this tells AI to turn toward station
            rotationRaw = stationAngle;

            targetBrake = (stationDist < 0.2f) ? 0.5f : 0.0f;
        }

        // Force balanced rotation in training data
        // 25% forced negative, 25% forced positive, 50% natural
        float rotationRoll = dist(rng);
        if (rotationRoll < 0.25f) {
            rotationRaw = -std::abs(rotationRaw);  // Force negative
            if (rotationRaw > -0.3f) rotationRaw = -0.5f;
        } else if (rotationRoll < 0.5f) {
            rotationRaw = std::abs(rotationRaw);   // Force positive
            if (rotationRaw < 0.3f) rotationRaw = 0.5f;
        }

        // Tanh outputs -1 to +1 directly, no conversion needed for strafe/rotation
        // Thrust and brake need to be mapped: tanh output -1 to +1 -> 0 to 1
        // So we store them as -1 to +1 and convert after prediction
        float targetThrustTanh = targetThrust * 2.0f - 1.0f;  // 0-1 -> -1 to +1
        float targetBrakeTanh = targetBrake * 2.0f - 1.0f;    // 0-1 -> -1 to +1

        example.target = Matrix(1, 4);
        example.target.data[0] = {
            std::max(-1.0f, std::min(1.0f, targetThrustTanh)),   // -1 to +1 (tanh)
            std::max(-1.0f, std::min(1.0f, strafeRaw)),          // -1 to +1 (tanh)
            std::max(-1.0f, std::min(1.0f, rotationRaw)),        // -1 to +1 (tanh)
            std::max(-1.0f, std::min(1.0f, targetBrakeTanh))     // -1 to +1 (tanh)
        };

        examples.push_back(example);
    }

    return examples;
}

void TrainingManager::initializeTrainingState()
{
    // Check if model already exists
    std::ifstream modelCheck("trained_model.nn");
    bool modelExists = modelCheck.good();
    modelCheck.close();

    if (modelExists) {
        std::cout << "Found existing trained model. Loading and continuing training...\n" << std::endl;
        network->loadModel("trained_model.nn");
    } else {
        std::cout << "No existing model found. Starting fresh training...\n" << std::endl;
    }

    std::cout << "Configuration:" << std::endl;
    std::cout << "  Training duration: " << TRAINING_TIME_SECONDS << " seconds" << std::endl;
    std::cout << "  Examples per batch: " << EXAMPLES_PER_BATCH << std::endl;
    std::cout << "  Learning rate: " << LEARNING_RATE << std::endl;
    std::cout << "  Progress update: every " << DISPLAY_INTERVAL_BATCHES << " batches" << std::endl;
    std::cout << "  Validation: " << VALIDATION_REQUIRED_WINS << "/" << VALIDATION_TESTS << " wins required to save" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "  Press 'Q' to stop and save best model" << std::endl;
    std::cout << "======================================\n" << std::endl;

    // Initialize log file
    std::ofstream logFile(TRAINING_LOG_FILE, std::ios::app);
    if (logFile.is_open()) {
        if (!modelExists) {
            logFile << "Batch #,Average Loss,Best Loss,Improvement" << std::endl;
        } else {
            logFile << "\n--- Continued Training Session ---\n";

            // Read the last best loss from the previous session
            std::ifstream readLog(TRAINING_LOG_FILE);
            if (readLog.is_open()) {
                std::string line;
                float lastBestLoss = std::numeric_limits<float>::max();
                while (std::getline(readLog, line)) {
                    if (line.empty() || line[0] == 'B' || line[0] == '-') continue;

                    size_t pos1 = line.find(',');
                    size_t pos2 = line.find(',', pos1 + 1);
                    if (pos2 != std::string::npos) {
                        try {
                            lastBestLoss = std::stof(line.substr(pos2 + 1));
                        } catch (...) {
                            continue;
                        }
                    }
                }
                if (lastBestLoss < std::numeric_limits<float>::max()) {
                    bestLoss = lastBestLoss;
                }
                readLog.close();
            }
        }
        logFile.close();
    }
}

void TrainingManager::logBatchProgress(float batchLoss, float validationLoss, float improvement)
{
    std::ofstream logFile(TRAINING_LOG_FILE, std::ios::app);
    if (logFile.is_open()) {
        logFile << totalBatches << "," << batchLoss << "," << bestLoss << "," << improvement << std::endl;
        logFile.flush();
        logFile.close();
    }
}

void TrainingManager::loadBestModel()
{
    if (std::ifstream(BEST_MODEL_FILE).good()) {
        network->loadModel(BEST_MODEL_FILE, false);
    }
}

void TrainingManager::updateBestModel(float validationLoss)
{
    if (validationLoss < bestLoss) {
        network->saveModel(BEST_MODEL_FILE, false);
    }
}

float TrainingManager::simulateGameFitness(int simulationFrames)
{
    // Use shared GameLogic for consistent behavior with game mode
    SimulationResult result = GameLogic::runSimulation(network, simulationFrames);

    // Store results for reporting
    lastSimWon = result.won;
    lastSimHit = result.hit;

    return result.totalLoss;
}

bool TrainingManager::validateModel(int numTests, int requiredWins, int maxFrames)
{
    int wins = 0;
    int hits = 0;
    float totalLoss = 0.0f;

    for (int i = 0; i < numTests; ++i) {
        SimulationResult result = GameLogic::runSimulation(network, maxFrames);
        if (result.won) wins++;
        if (result.hit) hits++;
        totalLoss += result.totalLoss;
    }

    float avgLoss = totalLoss / numTests;
    std::cout << "  Validation: " << wins << "/" << numTests << " wins, "
              << hits << " hits, avg loss: " << std::fixed << std::setprecision(2) << avgLoss;

    return wins >= requiredWins;
}

void TrainingManager::train()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Continuous Neural Network Training" << std::endl;
    std::cout << "   Space Station Target Behavior" << std::endl;
    std::cout << "========================================\n" << std::endl;

    initializeTrainingState();

    // Generate validation set
    std::vector<TrainingExample> validationSet = generateBatchData(EXAMPLES_PER_BATCH / 5);

    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastDisplayTime = startTime;

    bool stoppedEarly = false;
    while (true) {
        // Check for early stop (Q key)
        if (_kbhit()) {
            char key = _getch();
            if (key == 'q' || key == 'Q') {
                std::cout << "\n*** Stopping early - saving best model... ***\n";
                stoppedEarly = true;
                break;
            }
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();

        if (elapsedSeconds >= TRAINING_TIME_SECONDS) {
            break;
        }

        // Load best model at start of each batch
        loadBestModel();

        // Run a training batch
        totalBatches++;
        std::vector<TrainingExample> batchData = generateBatchData(EXAMPLES_PER_BATCH);
        float batchLoss = network->trainBatch(batchData, LEARNING_RATE);
        float validationLoss = network->calculateLoss(validationSet);

        float improvement = 0.0f;
        std::string evalMethod = "Valid";
        float gameFitness = 0.0f;
        bool showGameFitness = false;

        // Periodically evaluate using actual game simulation
        if (totalBatches % 100 == 0) {  // Evaluate less often for faster training
            gameFitness = simulateGameFitness(MAX_FRAMES);
            showGameFitness = true;

            if (lastSimWon) {
                // This model won once - but is it consistent?
                std::cout << "\n*** Single win detected - validating consistency... ***\n";
                bool isConsistent = validateModel(VALIDATION_TESTS, VALIDATION_REQUIRED_WINS, VALIDATION_MAX_FRAMES);

                if (isConsistent) {
                    std::cout << " - PASSED!\n";
                    if (!hasWinningModel) {
                        // First validated winning model
                        std::cout << "*** FIRST VALIDATED WIN! Saving model. ***\n";
                        hasWinningModel = true;
                        bestWinLoss = gameFitness;
                        bestLoss = gameFitness;
                        bestBatch = totalBatches;
                        network->saveModel("best_model.nn", false);
                        improvement = 1.0f;
                        evalMethod = "VALIDATED WIN (first!)";
                    } else if (gameFitness < bestWinLoss) {
                        // Better validated winner
                        std::cout << "*** BETTER VALIDATED WIN! " << gameFitness << " < " << bestWinLoss << " ***\n";
                        improvement = bestWinLoss - gameFitness;
                        bestWinLoss = gameFitness;
                        bestLoss = gameFitness;
                        bestBatch = totalBatches;
                        network->saveModel("best_model.nn", false);
                        evalMethod = "VALIDATED WIN (better)";
                    } else {
                        evalMethod = "VALIDATED WIN (not best)";
                    }
                } else {
                    std::cout << " - FAILED (lucky win)\n";
                    evalMethod = "LUCKY WIN (rejected)";
                }
            } else if (!hasWinningModel) {
                // No winning model yet - save based on lowest loss (temporary until we get a win)
                if (gameFitness < bestLoss) {
                    improvement = bestLoss - gameFitness;
                    bestLoss = gameFitness;
                    bestBatch = totalBatches;
                    network->saveModel("best_model.nn", false);
                    evalMethod = "Game (no win yet)";
                }
            }
            // If we have a winning model and this one didn't win, ignore it completely
        } else if (!hasWinningModel) {
            // Between game fitness evaluations, only track validation loss if no winner yet
            if (validationLoss < bestLoss) {
                improvement = bestLoss - validationLoss;
                bestLoss = validationLoss;
                evalMethod = "Valid";
            }
        }

        // Log to file every batch
        logBatchProgress(batchLoss, validationLoss, improvement);

        // Display progress periodically
        auto timeSinceDisplay = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastDisplayTime).count();
        if (timeSinceDisplay >= 5 || totalBatches % DISPLAY_INTERVAL_BATCHES == 0) {
            std::cout << "Batch " << std::setw(6) << totalBatches << " | "
                      << "Train Loss: " << std::fixed << std::setprecision(6) << batchLoss << " | ";

            // Show best winning loss if we have a winner, otherwise show best overall
            if (hasWinningModel) {
                std::cout << "Best WIN: " << std::setprecision(2) << bestWinLoss << " | ";
            } else {
                std::cout << "Best: " << std::setprecision(2) << bestLoss << " (no win) | ";
            }

            std::cout << "Time: " << std::setw(3) << elapsedSeconds << "s/" << TRAINING_TIME_SECONDS << "s | ";

            if (improvement > 0.0f) {
                std::cout << "Improved (" << evalMethod << ")";
            } else {
                std::cout << "No change";
            }

            if (showGameFitness) {
                std::cout << " | Fitness: " << std::fixed << std::setprecision(2) << gameFitness;
                if (lastSimWon) {
                    std::cout << " WIN!";
                } else if (lastSimHit) {
                    std::cout << " HIT";
                } else {
                    std::cout << " TIMEOUT";
                }
            }
            std::cout << std::endl;
            lastDisplayTime = currentTime;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();

    std::cout << "\n=======================================" << std::endl;
    if (stoppedEarly) {
        std::cout << "Training Stopped Early (Q pressed)" << std::endl;
    } else {
        std::cout << "Training Complete!" << std::endl;
    }
    std::cout << "=======================================" << std::endl;
    std::cout << "Total batches processed: " << totalBatches << std::endl;
    std::cout << "Total training time: " << totalTime << " seconds" << std::endl;
    std::cout << "Best validation loss: " << std::fixed << std::setprecision(6) << bestLoss << std::endl;
    std::cout << "Best performance at batch: " << bestBatch << std::endl;
    std::cout << "Average batches per second: " << (totalBatches / static_cast<float>(totalTime)) << std::endl;
    std::cout << "=======================================\n" << std::endl;

    // Save the best model as the final trained model
    if (std::ifstream(BEST_MODEL_FILE).good()) {
        // Load the best model and save it as trained_model.nn
        network->loadModel(BEST_MODEL_FILE, false);
        network->saveModel("trained_model.nn");
        std::cout << "Best winning model saved as trained_model.nn!" << std::endl;
    } else {
        // No best model found, save current network
        network->saveModel("trained_model.nn");
        std::cout << "Current model saved as trained_model.nn" << std::endl;
    }
}
