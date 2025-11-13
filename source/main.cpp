#include <windows.h>
#include <gdiplus.h>
#include <iostream>
using namespace std;
using namespace Gdiplus;

#pragma comment(lib, "Gdiplus.lib")

// Player position
int playerX = 100;
int playerY = 100;

// window size
int windowWidth = 800;
int windowHeight = 600;

// Track keys being pressed
bool keys[256] = { false };

// Flag to run the game loop
bool running = true;

// Global GDI+ objects
ULONG_PTR gdiplusToken;
GdiplusStartupInput gdiplusStartupInput;
Image* shipIdle = nullptr;

// Forward declare draw function
void Draw(HDC hdc);

// Update movement
void update() {
    if(playerX > 0){
        if (keys['A']) playerX -= 5;
    }
    if(playerX < windowWidth - 85){
        if (keys['D']) playerX += 5;
    }
    if(playerY > 0){
        if (keys['W']) playerY -= 5;
    }
    if(playerY < windowHeight - 50){
        if (keys['S']) playerY += 5;
    }

    cout << "X: " << playerX << " Y: " << playerY << endl;
}

// Game window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
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
    case WM_SIZE:
        windowWidth = LOWORD(lParam);
        windowHeight = HIWORD(lParam);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Main entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Initialize GDI+
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    shipIdle = new Image(L"Images/shipIdle.png"); // Load your image here

    // Register window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
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
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Main game loop (~60 FPS)
    const int FPS = 60;
    const int frameDelay = (1000 / FPS);

    MSG msg;
    while (running) {
        // Handle Windows messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        update();

        // Redraw window
        HDC hdc = GetDC(hwnd);
        RECT rect;
        GetClientRect(hwnd, &rect);
        windowWidth = rect.right - rect.left;
        windowHeight = rect.bottom - rect.top;
        Draw(hdc);
        ReleaseDC(hwnd, hdc);

        Sleep(frameDelay);
    }

    // Cleanup
    delete shipIdle;
    GdiplusShutdown(gdiplusToken);
    return 0;
}

// Draw everything here
void Draw(HDC hdc) {
    Graphics graphics(hdc);
    Bitmap buffer(windowWidth, windowHeight, &graphics);  // Offscreen buffer
    Graphics g(&buffer);

    // Fill background black
    g.Clear(Color(255, 0, 0, 0));

    // Draw the player image
    if (shipIdle)
        g.DrawImage(shipIdle, playerX, playerY, 84, 48);

    // Draw buffer to the screen
    graphics.DrawImage(&buffer, 0, 0, windowWidth, windowHeight);
}
