#pragma once

// ========== GAME SETTINGS ==========
// Shared between main.cpp and TrainingManager.cpp
// Change values here to affect both game and training

// Window dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int STATION_X = WINDOW_WIDTH / 2;
const int STATION_Y = WINDOW_HEIGHT / 2;

// Ship settings
const float MAX_SPEED = 5.0f;
const float THRUST_POWER = 0.15f;      // Acceleration per frame when thrusting (was 0.5)
const float STRAFE_POWER = 0.10f;      // Acceleration per frame when strafing (was 0.3)
const float DRAG_FACTOR = 0.98f;       // Constant drag applied each frame (1.0 = no drag)

// Bullet settings
const int BULLET_FIRE_RATE = 40;
const float BULLET_SPEED = 2.0f;
const float BULLET_COLLISION_RADIUS = 10.0f;
const double SAFE_ZONE_RADIUS = 100.0;  // No bullets fired when ship is this close to station
const double BULLET_PREDICTION_FACTOR = 0.7;  // How much to lead the target (0 = no prediction, 1 = full)

// Game duration
const int MAX_FRAMES = 5000;
