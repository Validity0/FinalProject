#pragma once
#include <vector>
#include <iostream>
#include "Vector2D.h"

class SpaceShip {
private:
    Vector2D pos;
    Vector2D velocity;
    int rotationAngle;
    int targetAngle;
    void addVelocity(const Vector2D);

public:
    SpaceShip();
    Vector2D getVelocity() const { return velocity; }
    void setVelocity(Vector2D vel) { velocity = vel; }
    int getRotationAngle() const { return rotationAngle; }
    void setRotationAngle(double angle) { rotationAngle = angle; }
    Vector2D getPosition() const { return pos; }
    void setPosition(Vector2D newPos) { pos = newPos; }

    // Update position based on velocity
    void updatePosition();

    // Clamp velocity to max speed
    void clampVelocity(float maxSpeed);

    // Apply friction/drag
    void applyDrag(float factor);

    //Applies force in the ship’s facing direction (forward backward)
    //#00
    void thrust(const double power);

    //Reduces Vector2D magnitude
    //#01
    bool brake(const int speed);

    //Rotates ship to target angle
    //Does not rotate velocity
    //#02
    bool rotate(const double toAngle);

    //Applies lateral force perpendicular to facing direction (left right)
    //#03
    void strafe(const int direction, const double power);
    
};