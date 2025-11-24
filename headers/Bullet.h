#pragma once
#include "Vector2D.h"
#include "SpaceShip.h"

class Bullet {
private:
    Vector2D pos;
    Vector2D velocity;
    SpaceShip* ship;

public:
    Bullet(Vector2D spawnloc, Vector2D velocity, SpaceShip* ship);

    // Check if bullet has collided with target ship
    bool hasHitShip() const;

    // Update bullet position based on velocity
    void update();

    // Position accessors
    Vector2D getPosition() const;
    void setPosition(Vector2D newPos);

    // Velocity accessors
    Vector2D getVelocity() const;
};