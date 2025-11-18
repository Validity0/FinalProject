#include "Spaceship.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double directionToAngle(int num)
{
    switch (num)
    {
    case 0: // forward (north)
        return -pi / 2.0;   // -90° in radians
    case 1: // right (east)
        return  pi / 2.0;   // 90° in radians
    case 2: // down (south)
        return -pi;         // -180° in radians
    case 3: // left (west)
        return 0.0;           // 0° in radians
    default:
        return -pi / 2.0;   // fallback: north
    }
}

SpaceShip::SpaceShip()
{
    pos = Vector2D();
    velocity = Vector2D();
    rotationAngle = 0;

    // Add spaceship to GDI+
}

void SpaceShip::addVelocity(Vector2D vel)
{
    velocity.add(vel);
}

void SpaceShip::thrust(const double power)
{
    // Convert degrees to radians, offset so 0° = north
    double angleRad = (rotationAngle - 90.0) * (M_PI / 180.0);

    // Compute thrust vector
    double a = std::cos(angleRad) * power; // X component
    double b = std::sin(angleRad) * power; // Y component

    Vector2D newVelocity(a, b);
    addVelocity(newVelocity);

    std::cout << "Thrust angle: " << rotationAngle
              << " | Velocity: " << velocity.print() << "\n";
}




bool SpaceShip::brake(int speed = 0)
{
    if(abs(velocity.getX() - speed) < 1 && abs(velocity.getY() - speed) < 1){
        std::cout << "Done";
        return true;
    }
    int x = 0; 
    int y = 0;
    if (abs(velocity.getY() - speed) > 0)
    {
        y = (velocity.getY() < speed) ? 1 : -1;
    }
    if (abs(velocity.getX() - speed) > 0)
    {
        x = (velocity.getX() < speed) ? 1 : -1;
    }
    std::cout << "(" << x << ", " << y << ") " << velocity.print() << std::endl;
    velocity.add(Vector2D(x, y));
    return false;
}

bool SpaceShip::rotate(const double targetAngle)
{
    if (abs(rotationAngle - targetAngle) > 6)
    {
        (rotationAngle < targetAngle) ? (rotationAngle += 5) : (rotationAngle -= 5);
        return false;
    }
    return true;
}

void SpaceShip::strafe(int direction, const double power)
{
    // direction: -1 = left, +1 = right

    // Convert rotationAngle (degrees) to radians
    double angleRad = (rotationAngle - 90.0) * (M_PI / 180.0);

    // Offset by ±90° for strafe
    double strafeAngle = angleRad + direction * (M_PI / 2.0);

    // Compute strafe vector
    double a = std::cos(strafeAngle) * power; // X component
    double b = std::sin(strafeAngle) * power; // Y component

    Vector2D newVelocity(a, b);
    addVelocity(newVelocity);

    std::cout << "Strafe angle: " << rotationAngle
              << " | Velocity: " << velocity.print() << "\n";
}
