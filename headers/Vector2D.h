#pragma once
#include <cmath>
#include <string>

const double pi = 2 * sin(1.0); 

class Vector2D{
private:
    double x;
    double y;

public:
    /*************************************
    * @brief Creates a 2D Vector with x and y maginitudes set to 0
    *
    *************************************/
    Vector2D();
    /*************************************
    * Creates a vector vector with x and y maginitudes
    * set to parameters
    *
    * @param x horizontal speed or a
    * @param y vertical speed or b
    *************************************/
    Vector2D(double x, double y);

    /*************************************
    * Adds other vector2d to self vector2d
    *
    * Negatives are allowed.
    *
    * @param vector Vector of vector to add to self
    *************************************/
    void add(const Vector2D vector);
    
    /*************************************
    * @brief Gets the horizontal or x component of
    * self vector
    *
    *************************************/
    double getX() const;
    /*************************************
    * @brief Gets the vertical or y component of
    * self vector
    *
    *************************************/
    double getY() const;

    /*************************************
    * @brief Checks if compared vector equals
    * self vector
    * 
    * @param other vector component to compare
    *************************************/
    bool equals(const Vector2D other) const;
        /*************************************
    * @brief Gets hypotenuse of Vector
    * 
    * @returns double speed
    *************************************/
    double speed() const;

    std::string print() const;
    
};