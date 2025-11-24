#pragma once
#include "Vector2D.h"
#include "SpaceShip.h"

class Bullet {
private:
    Vector2D pos;
    Vector2D velocity;
    
public:
    Bullet(Vector2D spawnloc, Vector2D velocity, SpaceShip* ship);

    bool hasHitShip() const;
};