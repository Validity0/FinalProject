#include "Velocity.h"

Velocity::Velocity() {
    x = 0;
    y = 0;
}

Velocity::Velocity(double x, double y) {
    this->x = x;
    this->y = y;
}

void Velocity::addVelocity(const Velocity velocity) {
    double vx = velocity.getXVelocity();
    double vy = velocity.getYVelocity();
    x += vx;
    y += vy;
}

double Velocity::getXVelocity() const {
    return x;
}

double Velocity::getYVelocity() const {
    return y;
}

bool Velocity::equals(const Velocity other) const {
    if (getXVelocity() != other.getXVelocity()){
        return false;
    } else if (getYVelocity() != other.getYVelocity()){
        return false;
    }
    return true;
}
