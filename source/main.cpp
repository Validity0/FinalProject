#define NOMINMAX
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <ctime>
#include <thread>
#include <iostream>
#include <vector>
#include <algorithm>
#include "SpaceShip.h"
#include "Bullet.h"
#include "NeuralNetwork.h"

// ========== Neural Network AI System ==========
NeuralNetwork* aiController = nullptr;
bool useAI = false;  // Toggle between manual and AI control

// Game state for AI input
struct GameState {
    double shipX;
    double shipY;
    double shipVelX;
    double shipVelY;
    double nearestThreatX;
    double nearestThreatY;
};

GameState currentGameState = {0, 0, 0, 0, 0, 0};

// Map neural network output to action
void executeAIAction(float aiOutput) {
    // Output range: 0.0 to 1.0
    if (aiOutput < 0.3f) {
        // Brake
    } else if (aiOutput < 0.5f) {
        // Thrust
    } else if (aiOutput < 0.75f) {
        // Strafe left
    } else {
        // Strafe right
    }
}

// ========== Game Engine ==========

// Action Tracking
bool calledAction{false};
std::vector<int> action; // {type, p1, p2, cooldownMs}
std::chrono::steady_clock::time_point nextActionTime = std::chrono::steady_clock::now();

// Player position
SpaceShip ship;
double playerX = 100;
double playerY = 100;
double lastPlayerX = -1;
double lastPlayerY = -1;

// Bullet management
std::vector<Bullet> bullets;
const int maxBullets = 100;

// Track keys being pressed
bool keys[256] = {false};

// Flag to run the game loop
bool running = true;

// Global GDI+ objects
ULONG_PTR gdiplusToken;
Gdiplus::GdiplusStartupInput gdiplusStartupInput;
Gdiplus::Image *shipIdle = nullptr;
Gdiplus::Image *shipBoost = nullptr;
Gdiplus::Image *shipLeft = nullptr;
Gdiplus::Image *shipRight = nullptr;

// Forward declare draw function
void Draw(HDC hdc, int width, int height);

void getShipAction(HWND hwnd)
{
    auto now = std::chrono::steady_clock::now();

    if (!calledAction && now >= nextActionTime)
    {
        if (keys['A'])
        {
            calledAction = true;
            action = {3, -1, 1, 150}; // dynamic cooldown
            nextActionTime = now + std::chrono::milliseconds(action[3]);
        }
        else if (keys['D'])
        {
            calledAction = true;
            action = {3, 1, 1, 150};
            nextActionTime = now + std::chrono::milliseconds(action[3]);
        }
        else if (keys['W'])
        {
            calledAction = true;
            action = {0, 1, 400};
            nextActionTime = now + std::chrono::milliseconds(action[3]);
        }
        else if (keys['S'])
        {
            calledAction = true;
            action = {1, 0};
        }
        else if (keys['Q'])
        {
            calledAction = true;
            action = {2, ship.getRotationAngle() - 15};
        }
        else if (keys['E'])
        {
            calledAction = true;
            action = {2, ship.getRotationAngle() + 15};
        }
    }
    else if (calledAction)
    {
        bool actionIsDone = false;

        switch (action[0])
        {
        case 0: // thrust
            actionIsDone = true;
            ship.thrust(action[1]);
            break;
        case 1: // brake
            actionIsDone = ship.brake(action[1]);
            break;
        case 2: // rotate
            actionIsDone = ship.rotate(action[1]);
            break;
        case 3: // strafe
            actionIsDone = true;
            ship.strafe(action[1], action[2]);
            break;
        }

        // End the action when its dynamic cooldown expires
        if (now >= nextActionTime && actionIsDone)
        {
            calledAction = false;
        }
    }

    // --- Update position using velocity ---

    HDC hdc = GetDC(hwnd);
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // --- Wrap horizontally ---
    if (playerX < 0)
        playerX = width;
    else if (playerX > width)
        playerX = 0;

    // --- Wrap vertically ---
    if (playerY < 0)
        playerY = height;
    else if (playerY > height)
        playerY = 0;
}

void updateShipPosition()
{
    Vector2D velocity = ship.getVelocity();
    double vx = velocity.getX() / 10.0;
    double vy = velocity.getY() / 10.0;

    playerX += vx;
    playerY += vy;

    ship.setPosition(Vector2D(playerX, playerY));
}

void updateBullets()
{
    // Update all bullets
    for (auto& bullet : bullets) {
        bullet.update();

        // Check collision with ship
        if (bullet.hasHitShip()) {
            bullet.setPosition(Vector2D(-1000, -1000)); // Remove off-screen
        }
    }

    // Remove bullets that are off-screen
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) {
                Vector2D pos = b.getPosition();
                return pos.getX() < -50 || pos.getX() > 850 ||
                       pos.getY() < -50 || pos.getY() > 650;
            }),
        bullets.end()
    );
}

// Game window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        keys[wParam] = true;
        if (wParam == 'T') {
            // Toggle AI control
            useAI = !useAI;
        }
        break;
    case WM_KEYUP:
        keys[wParam] = false;
        break;
    case WM_DESTROY:
        running = false;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Main entrypoint
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Initialize Neural Network
    aiController = new NeuralNetwork({6, 8, 4, 4});  // 6 inputs, 2 hidden layers, 4 output actions

    // Initialize GDI+
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    shipIdle = new Gdiplus::Image(L"Images/shipIdle.png");
    shipBoost = new Gdiplus::Image(L"Images/shipBoost.png");
    shipLeft = new Gdiplus::Image(L"Images/shipLeft.png");
    shipRight = new Gdiplus::Image(L"Images/shipRight.png");

    // Register window class
    WNDCLASSEX wc = {sizeof(WNDCLASSEX)};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = TEXT("MyGameWindow");
    RegisterClassEx(&wc);

    // Create window
    HWND hwnd = CreateWindowEx(
        0, TEXT("MyGameWindow"), TEXT("2D Space Game - Press T to toggle AI"),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Main game loop (~60 FPS)
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;

    MSG msg;
    while (running)
    {
        // Handle Windows messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (useAI && aiController) {
            // Update game state from current conditions
            currentGameState.shipX = playerX;
            currentGameState.shipY = playerY;
            currentGameState.shipVelX = ship.getVelocity().getX();
            currentGameState.shipVelY = ship.getVelocity().getY();

            // Prepare AI input matrix (6 inputs)
            Matrix aiInput(1, 6);
            aiInput.data[0] = {
                static_cast<float>(currentGameState.shipX / 800.0f),      // Normalized X
                static_cast<float>(currentGameState.shipY / 600.0f),      // Normalized Y
                static_cast<float>(currentGameState.shipVelX / 10.0f),    // Normalized Vel X
                static_cast<float>(currentGameState.shipVelY / 10.0f),    // Normalized Vel Y
                static_cast<float>(ship.getRotationAngle() / 360.0f),     // Rotation
                static_cast<float>(bullets.size() / static_cast<float>(maxBullets))  // Bullet count ratio
            };

            // Get AI decision
            Matrix prediction = aiController->predict(aiInput);
            executeAIAction(prediction.data[0][0]);
        } else {
            getShipAction(hwnd);
        }

        updateShipPosition();
        updateBullets();

        // Redraw window
        HDC hdc = GetDC(hwnd);
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        Draw(hdc, width, height);
        ReleaseDC(hwnd, hdc);

        Sleep(frameDelay);
    }

    // Cleanup
    delete aiController;
    delete shipIdle;
    delete shipBoost;
    delete shipLeft;
    delete shipRight;
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}

// Draw everything here
void Draw(HDC hdc, int width, int height)
{
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Bitmap buffer(width, height, &graphics);
    Gdiplus::Graphics g(&buffer);

    g.Clear(Gdiplus::Color(255, 0, 0, 0));

    // Draw ship
    if (shipIdle)
    {
        // Apply rotation around ship center
        g.TranslateTransform(playerX + 42, playerY + 24); // center of sprite
        g.RotateTransform(ship.getRotationAngle());
        g.TranslateTransform(-(playerX + 42), -(playerY + 24));

        g.DrawImage(shipIdle, playerX, playerY, 84, 48);

        // Reset transform
        g.ResetTransform();
    }

    // Draw bullets
    Gdiplus::SolidBrush yellowBrush(Gdiplus::Color(255, 255, 255, 0));
    for (const auto& bullet : bullets) {
        Vector2D pos = bullet.getPosition();
        g.FillEllipse(&yellowBrush, pos.getX() - 2, pos.getY() - 2, 4, 4);
    }

    // Draw AI status
    if (useAI) {
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 0, 255, 0));
        Gdiplus::Font font(L"Arial", 12);
        Gdiplus::PointF pointF(10, 10);
        g.DrawString(L"AI: ON", -1, &font, pointF, &textBrush);
    }

    graphics.DrawImage(&buffer, 0, 0, width, height);
}
