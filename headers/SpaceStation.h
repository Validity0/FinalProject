#pragma once
#include "Vector2D.h"

class SpaceStation {
private:
    Vector2D pos;
    int health;
    int fireCounter;
    const int FIRE_RATE = 20;  // Fire every 20 frames (MUST MATCH TRAINING)

public:
    SpaceStation(double centerX, double centerY);

    // Game state
    Vector2D getPosition() const { return pos; }
    int getHealth() const { return health; }
    bool isAlive() const { return health > 0; }

    // Gameplay
    void update();
    bool shouldFire() const { return fireCounter <= 0; }
    void resetFireCounter() { fireCounter = FIRE_RATE; }
    void takeDamage(int damage = 1) { health -= damage; }

    // Constants
    static const int RADIUS = 30;
};
