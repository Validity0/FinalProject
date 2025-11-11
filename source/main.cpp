#include <windows.h>
#include <gdiplus.h>
using namespace Gdiplus;
#define byte Gdiplus::Byte

#pragma comment(lib, "gdiplus.lib")

// Global variables
ULONG_PTR gdiplusToken;
int x = 400, y = 300;  // Circle position
bool keyW = false, keyA = false, keyS = false, keyD = false;
const int speed = 5;
const int WIN_WIDTH = 800, WIN_HEIGHT = 600;

// Forward declaration
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("GDIPlusWindow");
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    // Create window
    HWND hwnd = CreateWindowEx(
        0, TEXT("GDIPlusWindow"), TEXT("GDI+ Moving Circle (WASD)"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WIN_WIDTH, WIN_HEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Game loop
    MSG msg = {};
    while (true)
    {
        // Handle messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                goto exit;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Update movement
        if (keyW) y -= speed;
        if (keyS) y += speed;
        if (keyA) x -= speed;
        if (keyD) x += speed;

        // Keep circle in window bounds
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > WIN_WIDTH - 50) x = WIN_WIDTH - 50;
        if (y > WIN_HEIGHT - 50) y = WIN_HEIGHT - 50;

        // Redraw
        InvalidateRect(hwnd, nullptr, FALSE);
        Sleep(10); // ~60 FPS
    }

exit:
    GdiplusShutdown(gdiplusToken);
    return 0;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == 'W') keyW = true;
        if (wParam == 'A') keyA = true;
        if (wParam == 'S') keyS = true;
        if (wParam == 'D') keyD = true;
        break;

    case WM_KEYUP:
        if (wParam == 'W') keyW = false;
        if (wParam == 'A') keyA = false;
        if (wParam == 'S') keyS = false;
        if (wParam == 'D') keyD = false;
        break;

    case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // Create a GDI+ Graphics object from the window
    Gdiplus::Graphics graphics(hdc);

    // --- DOUBLE BUFFER START ---
    Gdiplus::Bitmap buffer(800, 600, &graphics);   // off-screen buffer
    Gdiplus::Graphics g(&buffer);                  // draw on buffer

    // Clear background
    g.Clear(Gdiplus::Color(255, 0, 0, 0)); // white

    // Draw your blue circle
    Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0, 255));
    g.FillEllipse(&brush, x, y, 50, 50);

    // Copy buffer to screen in one go
    graphics.DrawImage(&buffer, 0, 0);
    // --- DOUBLE BUFFER END ---

    EndPaint(hwnd, &ps);
    break;
    }
    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
