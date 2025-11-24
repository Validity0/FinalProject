#include "SpaceStation.h"
#include <cmath>

SpaceStation::SpaceStation(double centerX, double centerY)
    : pos(centerX, centerY), health(10), fireCounter(0)
{
}

void SpaceStation::update()
{
    // Decrement fire counter
    if (fireCounter > 0) {
        fireCounter--;
    }
}
