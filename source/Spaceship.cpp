#include "Spaceship.h"
#include <math.h>

SpaceShip::SpaceShip() {
    pos.x = 0.0;
    pos.y = 0.0;
    velocity = Velocity();
    rotationAngle = 0;

    //Add spaceship to GDI+
}

void SpaceShip::addVelocity(Velocity vel) {
    velocity.addVelocity(vel);
}

void SpaceShip::thrust(const ThrustDirection direction, const double power, const int seconds) {
    double rotationalAdjustment = directionToAngle(direction) + rotationAngle;
    
    double hypotenuse = power;
    double a;
    double b;

    double radians = rotationalAdjustment * pi / 180.0;
    a = cos(radians) * hypotenuse;
    b = sin(radians) * hypotenuse;

    Velocity newVelocity = Velocity(a, b);
    addVelocity(newVelocity);
}

void SpaceShip::brake(Velocity targetVelocity = Velocity()){
    //If the ship needs to slow down to avoid hitting something, brake either to 0 as default or to a target speed
    //Might have to 
    while (!velocity.equals(targetVelocity)){
        //Decrease Velocity
    }
}

void SpaceShip::rotate(const double targetAngle) {
    while (rotationAngle != targetAngle){
        //Make rotationAngle of ship equal targetAngle 
    } 
    
}

void SpaceShip::strafe(RotateDirection direction) {
    //Applies a force left or right of the ship's facing direction, instead of forward or backward.
}