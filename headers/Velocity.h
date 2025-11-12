#pragma once
using namespace std;

class Velocity{
private:
    double x;
    double y;

public:
    /*************************************
    * @brief Creates a velocity vector with x and y maginitudes set to 0
    *
    *************************************/
    Velocity();
    /*************************************
    * Creates a velocity vector with x and y maginitudes
    * set to parameters
    *
    * @param x horizontal speed or a
    * @param y vertical speed or b
    *************************************/
    Velocity(double x, double y);

    /*************************************
    * Adds other velocity to self velocity.
    *
    * Negatives are allowed.
    *
    * @param velocity Vector of Velocity to add to self
    *************************************/
    void addVelocity(const Velocity velocity);
    
    /*************************************
    * @brief Gets the horizontal or x component of
    * self vector
    *
    *************************************/
    double getXVelocity() const;
    /*************************************
    * @brief Gets the vertical or y component of
    * self vector
    *
    *************************************/
    double getYVelocity() const;

    /*************************************
    * @brief Checks if compared velocity equals
    * self velocity
    * 
    * @param other velocity component to compare
    *************************************/
    bool equals(const Velocity other) const;
    
};