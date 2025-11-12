<<<<<<< HEAD
#define NOMINMAX
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <ctime>
#include <thread>
#include <iostream>
#include "SpaceShip.h"

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

// Track keys being pressed
bool keys[256] = {false};

// Flag to run the game loop
bool running = true;

// Global GDI+ objects
ULONG_PTR gdiplusToken;
Gdiplus::GdiplusStartupInput gdiplusStartupInput;
Gdiplus::Image *shipIdle = nullptr;

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
    Vector2D velocity = ship.getVelocity(); // assume returns vector with getX(), getY()
    double vx = velocity.getX() / 10.0;
    double vy = velocity.getY() / 10.0;

    playerX += vx;
    playerY += vy;
}

// Game window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        keys[wParam] = true;
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

    // Initialize GDI+
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    shipIdle = new Gdiplus::Image(L"Images/shipIdle.png"); // Load your image here

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
        0, TEXT("MyGameWindow"), TEXT("My 2D Game"),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Main game loop (~60 FPS)
    const int FPS = 240;
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
        getShipAction(hwnd);
        updateShipPosition();

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
    delete shipIdle;
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

    graphics.DrawImage(&buffer, 0, 0, width, height);
}
=======
#include "NeuralNetwork.h"
#include <iostream>

using namespace std;

int main() {
    NeuralNetwork nn({3, 4, 4, 1});

    // One training example
    Matrix input(1, 3);
    input.data[0] = {0.9f, 0.3f, -0.7f}; // spaceship environment

    Matrix target(1, 1);
    target.data[0] = {1.0f}; // correct answer: thrust left

    float learningRate = 0.2f;

    // Training loop
    for (int i = 0; i < 10; ++i) {
        nn.train(input, target, learningRate);

        Matrix prediction = nn.predict(input);
        std::cout << "Step " << i + 1 << " prediction: ";
        prediction.print();
    }

    return 0;
}
>>>>>>> 2358869 (Neural Network 1.0)
