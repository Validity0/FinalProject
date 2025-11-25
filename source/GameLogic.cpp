#include "GameLogic.h"
#include <random>

SimulationResult GameLogic::runSimulation(NeuralNetwork* network, int maxFrames) {
    SpaceShip ship;
    std::vector<SimBullet> bullets;
    int bulletFireCounter = 0;

    // Randomize starting position along edges
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> edgePicker(0, 3);
    std::uniform_real_distribution<double> distX(50.0, WINDOW_WIDTH - 50.0);
    std::uniform_real_distribution<double> distY(50.0, WINDOW_HEIGHT - 50.0);

    double startX, startY;
    int edge = edgePicker(rng);
    switch (edge) {
        case 0: startX = distX(rng); startY = 50.0; break;
        case 1: startX = distX(rng); startY = WINDOW_HEIGHT - 50.0; break;
        case 2: startX = 50.0; startY = distY(rng); break;
        case 3: startX = WINDOW_WIDTH - 50.0; startY = distY(rng); break;
    }

    ship.setPosition(Vector2D(startX, startY));
    ship.setVelocity(Vector2D(0, 0));
    ship.setRotationAngle(0);

    float totalLoss = 0.0f;
    bool won = false;
    bool hit = false;
    float previousDistance = std::sqrt(
        (STATION_X - startX) * (STATION_X - startX) +
        (STATION_Y - startY) * (STATION_Y - startY)
    );

    // Track closest distance reached (for one-time proximity bonus)
    float closestDistanceReached = previousDistance;

    int frame;
    for (frame = 0; frame < maxFrames; ++frame) {
        Vector2D shipPos = ship.getPosition();
        double shipX = shipPos.getX();
        double shipY = shipPos.getY();

        // Fire bullets
        bulletFireCounter++;
        if (bulletFireCounter > BULLET_FIRE_RATE) {
            double distance = std::sqrt(
                (shipX - STATION_X) * (shipX - STATION_X) +
                (shipY - STATION_Y) * (shipY - STATION_Y)
            );

            if (distance > SAFE_ZONE_RADIUS) {
                Vector2D vel = ship.getVelocity();
                fireAtShip(shipX, shipY, vel.getX(), vel.getY(), bullets);
                bulletFireCounter = 0;
            }
        }

        // Update bullets
        updateBullets(bullets);

        // Check bullet collision
        if (checkBulletCollision(shipX, shipY, bullets)) {
            hit = true;
            break;
        }

        // Get AI decision and apply
        float rotationOutput;
        applyAIDecision(ship, network, shipX, shipY, bullets, rotationOutput);

        // Update ship physics
        ship.clampVelocity(MAX_SPEED);
        ship.updatePosition();

        // Wrap ship around screen
        shipPos = ship.getPosition();
        shipX = shipPos.getX();
        shipY = shipPos.getY();
        if (shipX < 0) shipX = WINDOW_WIDTH;
        else if (shipX > WINDOW_WIDTH) shipX = 0;
        if (shipY < 0) shipY = WINDOW_HEIGHT;
        else if (shipY > WINDOW_HEIGHT) shipY = 0;
        ship.setPosition(Vector2D(shipX, shipY));

        // Calculate distance to station
        double dx = STATION_X - shipX;
        double dy = STATION_Y - shipY;
        float currentDistance = std::sqrt(dx * dx + dy * dy);

        // Time penalty (encourages reaching station quickly)
        totalLoss += 0.15f;

        // Distance reward/penalty - STRONG incentive to get closer
        float distanceChange = currentDistance - previousDistance;
        totalLoss += distanceChange * 0.2f;  // Doubled penalty for moving away

        // Track closest approach for end-of-game bonus
        if (currentDistance < closestDistanceReached) {
            closestDistanceReached = currentDistance;
        }

        // Win condition - BIG reward
        if (currentDistance < 50.0f) {
            totalLoss -= 100.0f;  // Doubled win reward
            won = true;
            break;
        }

        previousDistance = currentDistance;
    }

    // End-of-game scoring based on closest distance reached (ONE-TIME, not per-frame)
    if (closestDistanceReached < 200.0f) totalLoss -= 5.0f;
    if (closestDistanceReached < 150.0f) totalLoss -= 10.0f;
    if (closestDistanceReached < 100.0f) totalLoss -= 20.0f;
    if (closestDistanceReached < 75.0f) totalLoss -= 30.0f;

    // Timeout penalty - harsh for not reaching station
    if (!won && !hit) {
        totalLoss += previousDistance * 0.3f;  // Increased timeout penalty
    }

    // Death penalty - significant but still allows learning from near-misses
    if (hit) {
        totalLoss += 50.0f;
    }

    return {won, hit, totalLoss, frame};
}

void GameLogic::fireAtShip(
    double shipX, double shipY,
    double shipVelX, double shipVelY,
    std::vector<SimBullet>& bullets
) {
    double dx = shipX - STATION_X;
    double dy = shipY - STATION_Y;
    double distance = std::sqrt(dx * dx + dy * dy);

    if (distance > 0) {
        double timeToHit = distance / BULLET_SPEED;
        // Cap prediction time to prevent overshooting
        if (timeToHit > 60.0) timeToHit = 60.0;

        // Predict future position
        double predictedX = shipX + shipVelX * timeToHit * BULLET_PREDICTION_FACTOR;
        double predictedY = shipY + shipVelY * timeToHit * BULLET_PREDICTION_FACTOR;

        double pdx = predictedX - STATION_X;
        double pdy = predictedY - STATION_Y;
        double pDist = std::sqrt(pdx * pdx + pdy * pdy);

        if (pDist > 0) {
            SimBullet bullet;
            bullet.x = STATION_X;
            bullet.y = STATION_Y;
            bullet.velX = (pdx / pDist) * BULLET_SPEED;
            bullet.velY = (pdy / pDist) * BULLET_SPEED;
            bullets.push_back(bullet);
        }
    }
}

void GameLogic::updateBullets(std::vector<SimBullet>& bullets) {
    for (auto& bullet : bullets) {
        bullet.x += bullet.velX;
        bullet.y += bullet.velY;

        // Wrap around screen
        if (bullet.x < 0) bullet.x += WINDOW_WIDTH;
        else if (bullet.x > WINDOW_WIDTH) bullet.x -= WINDOW_WIDTH;
        if (bullet.y < 0) bullet.y += WINDOW_HEIGHT;
        else if (bullet.y > WINDOW_HEIGHT) bullet.y -= WINDOW_HEIGHT;
    }
}

bool GameLogic::checkBulletCollision(
    double shipX, double shipY,
    const std::vector<SimBullet>& bullets
) {
    for (const auto& bullet : bullets) {
        double dx = bullet.x - shipX;
        double dy = bullet.y - shipY;
        double distance = std::sqrt(dx * dx + dy * dy);

        if (distance < BULLET_COLLISION_RADIUS) {
            return true;
        }
    }
    return false;
}

bool GameLogic::checkWin(double shipX, double shipY) {
    double dx = STATION_X - shipX;
    double dy = STATION_Y - shipY;
    double distance = std::sqrt(dx * dx + dy * dy);
    return distance < 50.0;
}

void GameLogic::applyAIDecision(
    SpaceShip& ship,
    NeuralNetwork* network,
    double shipX, double shipY,
    const std::vector<SimBullet>& bullets,
    float& rotationOutput
) {
    // Calculate station info
    double stationDx = STATION_X - shipX;
    double stationDy = STATION_Y - shipY;
    float stationDistance = std::sqrt(stationDx * stationDx + stationDy * stationDy);
    float stationAngle = std::atan2(stationDy, stationDx);

    // Find closest bullet
    float closestBulletDistance = 1000.0f;
    float closestBulletAngle = 0.0f;
    float closestBulletVelX = 0.0f;
    float closestBulletVelY = 0.0f;

    for (const auto& bullet : bullets) {
        double bdx = bullet.x - shipX;
        double bdy = bullet.y - shipY;
        float bDist = std::sqrt(bdx * bdx + bdy * bdy);

        if (bDist < closestBulletDistance) {
            closestBulletDistance = bDist;
            closestBulletAngle = std::atan2(bdy, bdx);
            closestBulletVelX = bullet.velX;
            closestBulletVelY = bullet.velY;
        }
    }

    // Build input
    Vector2D vel = ship.getVelocity();
    Matrix gameState(1, 12);
    gameState.data[0] = {
        static_cast<float>(shipX / WINDOW_WIDTH),
        static_cast<float>(shipY / WINDOW_HEIGHT),
        static_cast<float>(vel.getX() / 10.0f),
        static_cast<float>(vel.getY() / 10.0f),
        static_cast<float>(ship.getRotationAngle() / 360.0f),
        static_cast<float>(stationDistance / 500.0f),
        static_cast<float>(stationAngle / 3.14159f),
        static_cast<float>(closestBulletDistance / 500.0f),
        static_cast<float>(closestBulletAngle / 3.14159f),
        closestBulletVelX / 10.0f,
        closestBulletVelY / 10.0f,
        static_cast<float>(bullets.size() / 10.0f)
    };

    // Get prediction
    Matrix decision = network->predict(gameState);

    // Extract outputs - tanh gives -1 to +1 directly
    float thrustVal = (decision.data[0][0] + 1.0f) / 2.0f;
    thrustVal = std::max(0.0f, std::min(1.0f, thrustVal));

    float strafeVal = std::max(-1.0f, std::min(1.0f, decision.data[0][1]));
    float rotationVal = std::max(-1.0f, std::min(1.0f, decision.data[0][2]));

    float brakeVal = (decision.data[0][3] + 1.0f) / 2.0f;
    brakeVal = std::max(0.0f, std::min(1.0f, brakeVal));

    rotationOutput = rotationVal;

    // Apply to ship
    if (thrustVal > 0.1f) {
        ship.thrust(thrustVal * THRUST_POWER);
    }

    if (std::abs(strafeVal) > 0.1f) {
        ship.strafe(strafeVal > 0 ? 1 : -1, std::abs(strafeVal) * STRAFE_POWER);
    }

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
