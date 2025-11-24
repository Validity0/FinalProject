#include "Bullet.h"
#include <cmath>

Bullet::Bullet(Vector2D spawnloc, Vector2D velocity, SpaceShip* ship)
{
    pos = spawnloc;
    this->velocity = velocity;
    this->ship = ship;
}

bool Bullet::hasHitShip() const
{
    // Calculate distance between bullet and ship
    Vector2D shipPos = ship->getPosition();

    double dx = pos.getX() - shipPos.getX();
    double dy = pos.getY() - shipPos.getY();
    double distance = std::sqrt(dx * dx + dy * dy);

    // Collision radius: bullet (2) + ship (20)
    const double collisionThreshold = 22.0;

    return distance < collisionThreshold;
}

void Bullet::update()
{
    // Update bullet position using velocity
    pos.add(velocity);
}

Vector2D Bullet::getPosition() const
{
    return pos;
}

void Bullet::setPosition(Vector2D newPos)
{
    pos = newPos;
}

Vector2D Bullet::getVelocity() const
{
    return velocity;
}
