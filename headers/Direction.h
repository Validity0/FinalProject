#pragma once
#include <cmath>

const double pi = 2 * sin(1.0); 

typedef struct {
	double x, y;
} position;

enum class ThrustDirection {
	Forward,
	Backward
};

int directionToAngle(ThrustDirection dir) {
	switch (dir) {
		case ThrustDirection::Forward:  return 0;
		case ThrustDirection::Backward: return 180;
		default: return 0; // fallback
	}
}

enum class RotateDirection {
	Left, Right
};