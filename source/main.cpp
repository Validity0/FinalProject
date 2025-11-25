#include <iostream>
#include <vector>
#include <random>
#include <iomanip>
#include <fstream>
#include <cfloat>
#include <limits>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <string>
#include "SpaceShip.h"
#include "SpaceStation.h"
#include "Bullet.h"
#include "NeuralNetwork.h"
#include "TrainingManager.h"
#include "GameWindow.h"
#include "GameSettings.h"

// ========== GAME STATE ==========
SpaceShip ship;
SpaceStation station(STATION_X, STATION_Y);
std::vector<Bullet> enemyBullets;
NeuralNetwork* aiController = nullptr;
int bulletFireCounter = 0;  // Match training simulation counter

// Ship position is now stored in the ship object
// These are updated from ship.getPosition() for convenience
double playerX = 100;
double playerY = 100;

bool gameRunning = true;
bool gameWon = false;
bool gameLost = false;
std::string gameStatus = "Running";
ShipState currentShipState = ShipState::Idle;

// ========== GAME MECHANICS ==========

void updateShipPosition()
{
    // Update position using SpaceShip methods (SAME AS TRAINING)
    ship.clampVelocity(5.0f);
    ship.updatePosition();

    // Get position from ship
    Vector2D pos = ship.getPosition();
    playerX = pos.getX();
    playerY = pos.getY();

    // Wrap around screen
    if (playerX < 0) playerX = WINDOW_WIDTH;
    else if (playerX > WINDOW_WIDTH) playerX = 0;

    if (playerY < 0) playerY = WINDOW_HEIGHT;
    else if (playerY > WINDOW_HEIGHT) playerY = 0;

    // Update ship position
    ship.setPosition(Vector2D(playerX, playerY));
}

void updateEnemyBullets()
{
    // Update all bullets
    for (auto& bullet : enemyBullets) {
        bullet.update();

        // Wrap bullets around screen
        Vector2D bulletPos = bullet.getPosition();
        double bx = bulletPos.getX();
        double by = bulletPos.getY();
        if (bx < 0) bx += WINDOW_WIDTH;
        else if (bx > WINDOW_WIDTH) bx -= WINDOW_WIDTH;
        if (by < 0) by += WINDOW_HEIGHT;
        else if (by > WINDOW_HEIGHT) by -= WINDOW_HEIGHT;
        bullet.setPosition(Vector2D(bx, by));

        // Check collision with ship
        Vector2D shipPos(playerX, playerY);
        bulletPos = bullet.getPosition();
        double dx = bulletPos.getX() - shipPos.getX();
        double dy = bulletPos.getY() - shipPos.getY();
        double distance = std::sqrt(dx * dx + dy * dy);

        if (distance < BULLET_COLLISION_RADIUS) {
            gameLost = true;
            gameStatus = "LOST - Hit by bullet!";
            gameRunning = false;
        }
    }

    // Bullets wrap now, no need to remove off-screen ones
}

void updateStationFire()
{
    // Use same counter logic as training simulation
    bulletFireCounter++;
    if (bulletFireCounter > BULLET_FIRE_RATE) {
        // Fire bullet at spaceship (EXACTLY MATCHES TRAINING)
        double dx = playerX - STATION_X;
        double dy = playerY - STATION_Y;
        double distance = std::sqrt(dx * dx + dy * dy);

        if (distance > SAFE_ZONE_RADIUS) {
            // Predictive shooting - aim where ship will be
            Vector2D shipVel = ship.getVelocity();
            double timeToHit = distance / BULLET_SPEED;
            // Cap prediction time to prevent overshooting
            if (timeToHit > 60.0) timeToHit = 60.0;

            // Predict future position
            double predictedX = playerX + shipVel.getX() * timeToHit * BULLET_PREDICTION_FACTOR;
            double predictedY = playerY + shipVel.getY() * timeToHit * BULLET_PREDICTION_FACTOR;

            // Aim at predicted position
            double pdx = predictedX - STATION_X;
            double pdy = predictedY - STATION_Y;
            double pDist = std::sqrt(pdx * pdx + pdy * pdy);

            if (pDist > 0) {
                double vx = (pdx / pDist) * BULLET_SPEED;
                double vy = (pdy / pDist) * BULLET_SPEED;

                Vector2D stationPos(STATION_X, STATION_Y);
                Vector2D bulletVel(vx, vy);
                enemyBullets.push_back(Bullet(stationPos, bulletVel, &ship));
            }

            bulletFireCounter = 0;
        }
    }
}

void checkWinCondition()
{
    Vector2D stationPos = station.getPosition();
    double dx = playerX - stationPos.getX();
    double dy = playerY - stationPos.getY();
    double distance = std::sqrt(dx * dx + dy * dy);

    // Win if touching station (within radius)
    if (distance < SpaceStation::RADIUS + 20) {
        gameWon = true;
        gameStatus = "WON - Reached the station!";
        gameRunning = false;
    }
}

void processAIAction(float thrustVal, float strafeVal, float rotationVal, float brakeVal)
{
    // Clamp all outputs to valid ranges
    thrustVal = std::max(0.0f, std::min(1.0f, thrustVal));
    strafeVal = std::max(-1.0f, std::min(1.0f, strafeVal));
    rotationVal = std::max(-1.0f, std::min(1.0f, rotationVal));
    brakeVal = std::max(0.0f, std::min(1.0f, brakeVal));

    // Apply using SpaceShip methods (SAME AS TRAINING SIMULATION)
    // Thrust
    if (thrustVal > 0.1f) {
        ship.thrust(thrustVal * THRUST_POWER);
        currentShipState = ShipState::Boost;
    } else {
        currentShipState = ShipState::Idle;
    }

    // Strafe
    if (std::abs(strafeVal) > 0.1f) {
        ship.strafe(strafeVal > 0 ? 1 : -1, std::abs(strafeVal) * STRAFE_POWER);
        if (strafeVal < 0) {
            currentShipState = ShipState::Left;
        } else {
            currentShipState = ShipState::Right;
        }
    }

    // Rotation
    if (std::abs(rotationVal) > 0.05f) {
        ship.setRotationAngle(ship.getRotationAngle() + rotationVal * 6.0);
    }

    // Apply constant drag (ship slows down when not thrusting)
    ship.applyDrag(DRAG_FACTOR);

    // Additional braking on top of drag
    if (brakeVal > 0.1f) {
        ship.applyDrag(1.0f - brakeVal * 0.1f);
    }
}

void runGameMode()
{
    std::cout << "\n---------------------------------------" << std::endl;
    std::cout << "|         SPACE STATION GAME             |" << std::endl;
    std::cout << "|    AI Goal: Reach the Space Station    |" << std::endl;
    std::cout << "---------------------------------------\n" << std::endl;

    // Load best model (most up-to-date during training)
    if (!aiController->loadModel("best_model.nn")) {
        // Fall back to trained_model.nn if best_model doesn't exist
        if (!aiController->loadModel("trained_model.nn")) {
            std::cout << "Warning: Could not load any model. Using untrained network." << std::endl;
        } else {
            std::cout << "Loaded trained_model.nn" << std::endl;
        }
    } else {
        std::cout << "Loaded best_model.nn (most recent)" << std::endl;
    }

    // Create game window
    std::cout << "Creating game window..." << std::endl;
    GameWindow window(WINDOW_WIDTH, WINDOW_HEIGHT, "Space Station AI Game");
    if (!window.initialize()) {
        std::cout << "Error: Could not create game window." << std::endl;
        return;
    }
    std::cout << "Game window created successfully!" << std::endl;
    std::cout << "Press SPACE in the game window to start!\n" << std::endl;

    // Reset ship state and randomize starting position along edges (SAME AS TRAINING)
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> edgePicker(0, 3);  // 0=top, 1=bottom, 2=left, 3=right
    std::uniform_real_distribution<double> distX(50.0, WINDOW_WIDTH - 50.0);
    std::uniform_real_distribution<double> distY(50.0, WINDOW_HEIGHT - 50.0);

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
    ship.setVelocity(Vector2D(0, 0));  // Reset velocity to zero!
    ship.setRotationAngle(0);
    playerX = startX;
    playerY = startY;
    enemyBullets.clear();  // Clear any bullets
    bulletFireCounter = 0;  // Reset fire counter (same as training starts at 0)
    gameRunning = true;
    gameWon = false;
    gameLost = false;
    gameStatus = "Running";

    // Wait for space key to start
    bool gameStarted = false;
    while (!gameStarted && window.isOpen()) {
        window.processMessages();

        // Render initial frame (frozen)
        window.render(playerX, playerY, ship.getRotationAngle(), station, enemyBullets, 0, "Press SPACE to start", currentShipState);

        // Check for space key
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            gameStarted = true;
            std::cout << "Game started! Close the window to end the game.\n" << std::endl;
        }

        Sleep(16);
    }

    int frameCount = 0;
    const int MAX_FRAMES = 5000;

    while (gameRunning && frameCount < MAX_FRAMES && window.isOpen()) {
        frameCount++;

        // Handle window messages
        window.processMessages();

        // MATCH TRAINING ORDER: bullets fire -> bullets update -> AI decision -> ship physics -> win check
        // Fire and update bullets FIRST
        updateStationFire();
        updateEnemyBullets();

        // Check if hit by bullet (game over)
        if (gameLost) break;

        // NOW AI makes decision based on current bullet positions
        // Calculate station info
        double stationDx = STATION_X - playerX;
        double stationDy = STATION_Y - playerY;
        float stationDistance = std::sqrt(stationDx * stationDx + stationDy * stationDy);
        float stationAngle = std::atan2(stationDy, stationDx);  // Absolute angle (same as training)

        // Find closest bullet
        float closestBulletDistance = 1000.0f;
        float closestBulletAngle = 0.0f;
        float closestBulletVelX = 0.0f;
        float closestBulletVelY = 0.0f;

        for (const auto& bullet : enemyBullets) {
            Vector2D bPos = bullet.getPosition();
            double bdx = bPos.getX() - playerX;
            double bdy = bPos.getY() - playerY;
            float bDist = std::sqrt(bdx * bdx + bdy * bdy);

            if (bDist < closestBulletDistance) {
                closestBulletDistance = bDist;
                closestBulletAngle = std::atan2(bdy, bdx);  // Absolute angle (same as training)
                Vector2D bVel = bullet.getVelocity();
                closestBulletVelX = bVel.getX();
                closestBulletVelY = bVel.getY();
            }
        }

        // Build 12-input sensor array (EXACTLY MATCHES TRAINING SIMULATION)
        Vector2D vel = ship.getVelocity();
        Matrix gameState(1, 12);
        gameState.data[0] = {
            static_cast<float>(playerX / WINDOW_WIDTH),              // 0: Ship X
            static_cast<float>(playerY / WINDOW_HEIGHT),             // 1: Ship Y
            static_cast<float>(vel.getX() / 10.0f),                  // 2: Ship velocity X
            static_cast<float>(vel.getY() / 10.0f),                  // 3: Ship velocity Y
            static_cast<float>(ship.getRotationAngle() / 360.0f),      // 4: Ship rotation (normalized)
            static_cast<float>(stationDistance / 500.0f),             // 5: Station distance
            static_cast<float>(stationAngle / 3.14159f),              // 6: Station angle
            static_cast<float>(closestBulletDistance / 500.0f),       // 7: Closest bullet distance
            static_cast<float>(closestBulletAngle / 3.14159f),        // 8: Closest bullet angle
            static_cast<float>(closestBulletVelX / 10.0f),            // 9: Closest bullet vel X
            static_cast<float>(closestBulletVelY / 10.0f),            // 10: Closest bullet vel Y
            static_cast<float>(enemyBullets.size() / 10.0f)           // 11: Number of bullets
        };

        Matrix decision = aiController->predict(gameState);
        // Extract 4 outputs - tanh gives -1 to +1 directly
        // Thrust and brake: convert from -1,+1 to 0,1
        float thrustVal = (decision.data[0][0] + 1.0f) / 2.0f;
        float strafeVal = decision.data[0][1];   // Already -1 to +1
        float rotationVal = decision.data[0][2]; // Already -1 to +1
        float brakeVal = (decision.data[0][3] + 1.0f) / 2.0f;

        // Debug output every 10 frames
        if (frameCount % 10 == 0) {
            std::cout << "Frame " << frameCount << " | Rotation (tanh): " << rotationVal << std::endl;
        }

        processAIAction(thrustVal, strafeVal, rotationVal, brakeVal);

        // Update ship physics (same as training)
        updateShipPosition();

        // Check win condition
        checkWinCondition();

        // Render game
        window.render(playerX, playerY, ship.getRotationAngle(), station, enemyBullets, frameCount, gameStatus, currentShipState);

        // Progress indicator
        if (frameCount % 500 == 0) {
            std::cout << "Frame " << frameCount << " | "
                      << "Ship: (" << static_cast<int>(playerX) << ", " << static_cast<int>(playerY) << ") | "
                      << "Station: (" << STATION_X << ", " << STATION_Y << ") | "
                      << "Bullets: " << enemyBullets.size() << std::endl;
        }

        // Limit frame rate
        Sleep(16);  // ~60 FPS
    }

    // Game over
    std::cout << "\n---------------------------------------" << std::endl;
    std::cout << "Game Over!" << std::endl;
    std::cout << "Status: " << gameStatus << std::endl;
    std::cout << "Frames played: " << frameCount << std::endl;
    std::cout << "Final position: (" << static_cast<int>(playerX) << ", " << static_cast<int>(playerY) << ")" << std::endl;
    std::cout << "---------------------------------------\n" << std::endl;

    if (gameWon) {
        std::cout << "SUCCESS! The AI reached the Space Station!" << std::endl;
    } else if (gameLost) {
        std::cout << "FAILED! The AI was destroyed." << std::endl;
    } else {
        std::cout << "TIME LIMIT! The AI ran out of time." << std::endl;
    }
    std::cout << std::endl;
}

void runTrainingMode()
{
    TrainingManager trainer(aiController);
    trainer.train();
}

// ========== MAIN ==========

int main() {
    std::cout << "\n---------------------------------------" << std::endl;
    std::cout << "|   SPACE STATION AI GAME & TRAINER      |" << std::endl;
    std::cout << "---------------------------------------\n" << std::endl;

    // Initialize neural network (12 inputs, 4 outputs: thrust, strafe, rotation, brake)
    aiController = new NeuralNetwork({12, 32, 16, 4});

    std::cout << "Select mode:" << std::endl;
    std::cout << "1) Training - Train AI to reach station" << std::endl;
    std::cout << "2) Game - Watch trained AI play" << std::endl;
    std::cout << "Enter choice (1 or 2): ";

    int choice;
    std::cin >> choice;

    std::cout << std::endl;

    if (choice == 1) {
        runTrainingMode();
    } else if (choice == 2) {
        runGameMode();
    } else {
        std::cout << "Invalid choice. Running training mode..." << std::endl;
        runTrainingMode();
    }

    // Cleanup
    delete aiController;

    std::cout << "Program complete." << std::endl;
    return 0;
}
