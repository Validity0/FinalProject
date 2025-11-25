#pragma once
#include "SpaceShip.h"
#include "GameSettings.h"
#include "NeuralNetwork.h"
#include <vector>
#include <cmath>

// Bullet structure for simulation
struct SimBullet {
    double x, y;
    double velX, velY;
};

// Game simulation result
struct SimulationResult {
    bool won;
    bool hit;
    float totalLoss;
    int framesPlayed;
};

class GameLogic {
public:
    // Run a complete simulation and return the result
    static SimulationResult runSimulation(NeuralNetwork* network, int maxFrames);

    // Process a single frame - returns true if game should continue
    static bool processFrame(
        SpaceShip& ship,
        std::vector<SimBullet>& bullets,
        int& bulletFireCounter,
        NeuralNetwork* network,
        float& totalLoss,
        bool& won,
        bool& hit
    );

    // Fire bullet with prediction
    static void fireAtShip(
        double shipX, double shipY,
        double shipVelX, double shipVelY,
        std::vector<SimBullet>& bullets
    );

    // Update bullets (move + wrap)
    static void updateBullets(std::vector<SimBullet>& bullets);

    // Check bullet collision with ship
    static bool checkBulletCollision(
        double shipX, double shipY,
        const std::vector<SimBullet>& bullets
    );

    // Check win condition
    static bool checkWin(double shipX, double shipY);

    // Get AI decision and apply to ship
    static void applyAIDecision(
        SpaceShip& ship,
        NeuralNetwork* network,
        double shipX, double shipY,
        const std::vector<SimBullet>& bullets,
        float& rotationOutput  // Output for debugging
    );
};
