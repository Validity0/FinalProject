#pragma once
#include <vector>
#include "Velocity.h"
#include "Direction.h"

using namespace std;

class SpaceShip {
private:
    position pos;
    Velocity velocity;
    double rotationAngle;
    double targetAngle;
    void addVelocity(const Velocity);
    void checkRotation();

public:
    SpaceShip();

    //Applies force in the ship’s facing direction (forward backward)
    void thrust(const ThrustDirection direction, const double power, const int seconds);

    //Reduces velocity magnitude
    void brake(const Velocity targetVelocity);

    //Changes facing angle, not velocity
    void rotate(const double toAngle);

    //Applies lateral force perpendicular to facing direction (left right)
    void strafe(const RotateDirection);
    
};