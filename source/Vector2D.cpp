#include "Vector2d.h"
#include <cmath>

Vector2D::Vector2D() {
    x = 0;
    y = 0;
}

Vector2D::Vector2D(double x, double y) {
    this->x = x;
    this->y = y;
}

void Vector2D::add(const Vector2D vector) {
    double vx = vector.getX();
    double vy = vector.getY();
    x += vx;
    y += vy;
}

double Vector2D::getX() const {
    return x;
}

double Vector2D::getY() const {
    return y;
}

bool Vector2D::equals(const Vector2D other) const {
    if (getX() != other.getX()){
        return false;
    } else if (getY() != other.getY()){
        return false;
    }
    return true;
}

double Vector2D::speed() const {
    return sqrt(x * x + y * y);
};

std::string Vector2D::print() const {
    return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
}