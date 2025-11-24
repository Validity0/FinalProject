#include "TrainingManager.h"
#include "SpaceShip.h"
#include <iostream>
#include <fstream>
#include <random>
#include <iomanip>
#include <limits>
#include <chrono>
#include <cmath>

// Game parameters for training data (MUST MATCH main.cpp)
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int STATION_X = WINDOW_WIDTH / 2;
const int STATION_Y = WINDOW_HEIGHT / 2;
const float MAX_SPEED = 5.0f;
const int BULLET_FIRE_RATE = 20;
const float BULLET_SPEED = 3.0f;

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
        // Use heuristic: move toward station, avoid bullets
        float targetThrust = (stationDist > 0.3f) ? 0.7f : 0.2f;
        float targetStrafe = std::sin(stationAngle) * 0.5f;
        float targetRotation = stationAngle;  // Stronger rotation signal
        float targetBrake = (closestBulletDist < 0.3f) ? 0.8f : 0.0f;

        example.target = Matrix(1, 4);
        example.target.data[0] = {
            std::max(0.0f, std::min(1.0f, targetThrust)),
            std::max(-1.0f, std::min(1.0f, targetStrafe)),
            std::max(-1.0f, std::min(1.0f, targetRotation)),
            std::max(0.0f, std::min(1.0f, targetBrake))
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
    // Simulate actual gameplay using SpaceShip class (SAME AS GAME MODE)
    SpaceShip ship;

    // Randomize starting position along edges (SAME AS GAME MODE)
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> edgePicker(0, 3);  // 0=top, 1=bottom, 2=left, 3=right
    std::uniform_real_distribution<double> distX(50.0, WINDOW_WIDTH - 50.0);
    std::uniform_real_distribution<double> distY(50.0, WINDOW_HEIGHT - 50.0);

    double stationX = WINDOW_WIDTH / 2.0;
    double stationY = WINDOW_HEIGHT / 2.0;

    // Spawn along a random edge
    double startX, startY;
    int edge = edgePicker(rng);
    switch (edge) {
        case 0:  // Top edge
            startX = distX(rng);
            startY = 50.0;
            break;
        case 1:  // Bottom edge
            startX = distX(rng);
            startY = WINDOW_HEIGHT - 50.0;
            break;
        case 2:  // Left edge
            startX = 50.0;
            startY = distY(rng);
            break;
        case 3:  // Right edge
            startX = WINDOW_WIDTH - 50.0;
            startY = distY(rng);
            break;
    }

    ship.setPosition(Vector2D(startX, startY));

    float totalLoss = 0.0f;
    float previousDistance = std::sqrt((stationX - startX) * (stationX - startX) +
                                        (stationY - startY) * (stationY - startY));
    bool aiWon = false;
    bool aiHit = false;

    // Bullet simulation
    std::vector<std::pair<double, double>> bullets;
    std::vector<std::pair<float, float>> bulletVels;
    int bulletFireCounter = 0;

    for (int frame = 0; frame < simulationFrames; ++frame) {
        Vector2D shipPos = ship.getPosition();
        double shipX = shipPos.getX();
        double shipY = shipPos.getY();

        // Fire bullets from station periodically (SAME RATE AS GAME)
        bulletFireCounter++;
        if (bulletFireCounter > BULLET_FIRE_RATE) {
            double dx = shipX - stationX;
            double dy = shipY - stationY;
            double distance = std::sqrt(dx * dx + dy * dy);

            if (distance > 0) {
                float velX = (dx / distance) * BULLET_SPEED;
                float velY = (dy / distance) * BULLET_SPEED;
                bullets.push_back({stationX, stationY});
                bulletVels.push_back({velX, velY});
                bulletFireCounter = 0;
            }
        }

        // Update bullets
        for (size_t i = 0; i < bullets.size(); ++i) {
            bullets[i].first += bulletVels[i].first;
            bullets[i].second += bulletVels[i].second;

            // Check collision with ship
            double bx = bullets[i].first - shipX;
            double by = bullets[i].second - shipY;
            double bulletDist = std::sqrt(bx * bx + by * by);

            if (bulletDist < 20.0) {
                // GAME OVER on hit - same as real game!
                aiHit = true;
                totalLoss += 10.0f;  // Heavy penalty
                // End simulation immediately like the real game
                break;
            }
        }

        // If hit, end simulation (same as game)
        if (aiHit) break;

        // Remove off-screen bullets
        for (size_t i = 0; i < bullets.size(); ++i) {
            if (bullets[i].first < -50 || bullets[i].first > WINDOW_WIDTH + 50 ||
                bullets[i].second < -50 || bullets[i].second > WINDOW_HEIGHT + 50) {
                bullets.erase(bullets.begin() + i);
                bulletVels.erase(bulletVels.begin() + i);
                i--;
            }
        }

        // Create game state (EXACTLY MATCHES GAME MODE)
        double stationDx = stationX - shipX;
        double stationDy = stationY - shipY;
        float stDist = std::sqrt(stationDx * stationDx + stationDy * stationDy);
        float stAngle = std::atan2(stationDy, stationDx);

        // Find closest bullet
        float closestBDist = 1000.0f;
        float closestBAngle = 0.0f;
        float closestBVelX = 0.0f;
        float closestBVelY = 0.0f;

        for (size_t i = 0; i < bullets.size(); ++i) {
            double bdx = bullets[i].first - shipX;
            double bdy = bullets[i].second - shipY;
            float bDist = std::sqrt(bdx * bdx + bdy * bdy);

            if (bDist < closestBDist) {
                closestBDist = bDist;
                closestBAngle = std::atan2(bdy, bdx);
                closestBVelX = bulletVels[i].first;
                closestBVelY = bulletVels[i].second;
            }
        }

        Vector2D vel = ship.getVelocity();
        Matrix gameState(1, 12);
        gameState.data[0] = {
            static_cast<float>(shipX / WINDOW_WIDTH),              // 0: Ship X
            static_cast<float>(shipY / WINDOW_HEIGHT),             // 1: Ship Y
            static_cast<float>(vel.getX() / 10.0f),                // 2: Ship velocity X
            static_cast<float>(vel.getY() / 10.0f),                // 3: Ship velocity Y
            0.0f,                                                   // 4: Ship rotation
            static_cast<float>(stDist / 500.0f),                   // 5: Station distance
            static_cast<float>(stAngle / 3.14159f),                // 6: Station angle
            static_cast<float>(closestBDist / 500.0f),             // 7: Closest bullet distance
            static_cast<float>(closestBAngle / 3.14159f),          // 8: Closest bullet angle
            closestBVelX / 10.0f,                                   // 9: Closest bullet vel X
            closestBVelY / 10.0f,                                   // 10: Closest bullet vel Y
            static_cast<float>(bullets.size() / 10.0f)             // 11: Number of bullets
        };

        // Get AI decision
        Matrix decision = network->predict(gameState);

        // Extract outputs and apply using SpaceShip methods (SAME AS GAME MODE)
        float thrustVal = std::max(0.0f, std::min(1.0f, decision.data[0][0]));
        float strafeVal = std::max(-1.0f, std::min(1.0f, decision.data[0][1]));
        float rotationVal = std::max(-1.0f, std::min(1.0f, decision.data[0][2]));
        float brakeVal = std::max(0.0f, std::min(1.0f, decision.data[0][3]));

        // Apply thrust using SpaceShip method
        if (thrustVal > 0.1f) {
            ship.thrust(thrustVal * 0.5);
        }

        // Apply strafe using SpaceShip method
        if (std::abs(strafeVal) > 0.1f) {
            ship.strafe(strafeVal > 0 ? 1 : -1, std::abs(strafeVal) * 0.3);
        }

        // Apply rotation using SpaceShip method
        if (std::abs(rotationVal) > 0.05f) {
            ship.setRotationAngle(ship.getRotationAngle() + rotationVal * 6.0);
        }

        // Apply brake
        if (brakeVal > 0.1f) {
            ship.applyDrag(1.0f - brakeVal * 0.1f);
        }

        // Clamp velocity and update position
        ship.clampVelocity(MAX_SPEED);
        ship.updatePosition();

        // Wrap around screen
        shipPos = ship.getPosition();
        shipX = shipPos.getX();
        shipY = shipPos.getY();
        if (shipX < 0) shipX = WINDOW_WIDTH;
        else if (shipX > WINDOW_WIDTH) shipX = 0;
        if (shipY < 0) shipY = WINDOW_HEIGHT;
        else if (shipY > WINDOW_HEIGHT) shipY = 0;
        ship.setPosition(Vector2D(shipX, shipY));

        // Calculate distance to station
        double dx = stationX - shipX;
        double dy = stationY - shipY;
        float currentDistance = std::sqrt(dx * dx + dy * dy);

        // Time penalty - every frame costs something (encourages fast completion)
        totalLoss += 0.02f;

        // Reward for moving closer to station
        // If distance decreased, reward (negative loss). If it increased, penalty (positive loss)
        float distanceChange = currentDistance - previousDistance;
        if (distanceChange < 0) {
            // Getting closer = good
            totalLoss += distanceChange * 0.01f; // Negative = reward
        } else {
            // Getting farther = bad
            totalLoss += distanceChange * 0.02f; // Positive = penalty (higher weight)
        }

        // Bonus for being very close
        if (currentDistance < 100.0f) {
            totalLoss -= 0.1f; // Small continuous reward for proximity
        }

        // Win condition
        if (currentDistance < 50.0f) {
            totalLoss -= 5.0f; // Big reward for reaching (increased)
            aiWon = true;
            break;
        }

        previousDistance = currentDistance;
    }

    // Store results for reporting
    lastSimWon = aiWon;
    lastSimHit = aiHit;

    // Timeout penalty - if didn't win, add penalty based on final distance
    if (!aiWon && !aiHit) {
        totalLoss += previousDistance * 0.01f;  // Penalty for how far away when time ran out
    }

    // Return loss (lower = better, can be negative for very good performance)
    return totalLoss;
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

    while (true) {
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
        if (totalBatches % 20 == 0) {
            gameFitness = simulateGameFitness(500);
            showGameFitness = true;

            if (lastSimWon) {
                // This model won!
                if (!hasWinningModel) {
                    // First winning model - save it regardless of loss
                    hasWinningModel = true;
                    bestWinLoss = gameFitness;
                    bestLoss = gameFitness;
                    bestBatch = totalBatches;
                    network->saveModel("best_model.nn", false);
                    improvement = 1.0f;
                    evalMethod = "WIN (first!)";
                } else if (gameFitness < bestWinLoss) {
                    // Multiple wins - only save if lower loss than previous winners
                    improvement = bestWinLoss - gameFitness;
                    bestWinLoss = gameFitness;
                    bestLoss = gameFitness;
                    bestBatch = totalBatches;
                    network->saveModel("best_model.nn", false);
                    evalMethod = "WIN (better)";
                } else {
                    evalMethod = "WIN (not best)";
                }
            } else if (!hasWinningModel) {
                // No winning model yet - save based on lowest loss
                if (gameFitness < bestLoss) {
                    improvement = bestLoss - gameFitness;
                    bestLoss = gameFitness;
                    bestBatch = totalBatches;
                    network->saveModel("best_model.nn", false);
                    evalMethod = "Game";
                }
            }
            // If we have a winning model and this one didn't win, don't save it
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
                      << "Train Loss: " << std::fixed << std::setprecision(6) << batchLoss << " | "
                      << "Best Loss: " << bestLoss << " | "
                      << "Time: " << std::setw(2) << elapsedSeconds << "s/" << TRAINING_TIME_SECONDS << "s | ";

            if (improvement > 0.0f) {
                std::cout << "Improved (" << evalMethod << ")";
            } else {
                std::cout << "No change";
            }

            if (showGameFitness) {
                std::cout << " | Game Fitness: " << std::fixed << std::setprecision(4) << gameFitness;
                if (lastSimWon) {
                    std::cout << " | WIN!";
                } else if (lastSimHit) {
                    std::cout << " | HIT";
                } else {
                    std::cout << " | TIMEOUT";
                }
            }
            std::cout << std::endl;
            lastDisplayTime = currentTime;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();

    std::cout << "\n=======================================" << std::endl;
    std::cout << "Training Complete!" << std::endl;
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
