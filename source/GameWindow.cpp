#include "GameWindow.h"
#include <iostream>
#include <sstream>
#include <iomanip>

// Global pointer for window class to access instance
static GameWindow* g_pThis = nullptr;

// Convert string to wide string for Windows API
std::wstring toWideString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

LRESULT CALLBACK GameWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        GameWindow* pThis = reinterpret_cast<GameWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        return 0;
    }

    GameWindow* pThis = nullptr;
    if (msg == WM_CREATE) {
        pThis = g_pThis;
    } else {
        pThis = reinterpret_cast<GameWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GameWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CLOSE:
            isRunning = false;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

GameWindow::GameWindow(int width, int height, const char* title)
    : hwnd(nullptr), windowWidth(width), windowHeight(height), gdiplusToken(0), isRunning(true),
      backBufferDC(nullptr), backBufferBitmap(nullptr), oldBitmap(nullptr),
      lastShipState(ShipState::Idle)
{
    // Initialize GDI+
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "GameWindowClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    // Create window
    hwnd = CreateWindowEx(
        0,
        "GameWindowClass",
        title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth + 16, windowHeight + 39,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        // Bring window to foreground
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);

        // Create double buffer
        HDC screenDC = GetDC(hwnd);
        backBufferDC = CreateCompatibleDC(screenDC);
        backBufferBitmap = CreateCompatibleBitmap(screenDC, windowWidth, windowHeight);
        oldBitmap = (HBITMAP)SelectObject(backBufferDC, backBufferBitmap);
        ReleaseDC(hwnd, screenDC);
    }

    // Load ship images
    loadImages();
}

GameWindow::~GameWindow()
{
    // Clean up double buffer
    if (backBufferDC) {
        if (oldBitmap) SelectObject(backBufferDC, oldBitmap);
        if (backBufferBitmap) DeleteObject(backBufferBitmap);
        DeleteDC(backBufferDC);
    }

    if (hwnd) {
        DestroyWindow(hwnd);
    }
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

bool GameWindow::initialize()
{
    return hwnd != nullptr;
}

bool GameWindow::loadImages()
{
    // Load ship images from Images folder
    shipIdle = std::make_unique<Gdiplus::Image>(toWideString("Images\\shipIdle.png").c_str());
    shipBoost = std::make_unique<Gdiplus::Image>(toWideString("Images\\shipBoost.png").c_str());
    shipLeft = std::make_unique<Gdiplus::Image>(toWideString("Images\\shipLeft.png").c_str());
    shipRight = std::make_unique<Gdiplus::Image>(toWideString("Images\\shipRight.png").c_str());

    return shipIdle && shipBoost && shipLeft && shipRight;
}

Gdiplus::Image* GameWindow::getShipImage(ShipState state)
{
    switch (state) {
        case ShipState::Boost:
            return shipBoost.get();
        case ShipState::Left:
            return shipLeft.get();
        case ShipState::Right:
            return shipRight.get();
        case ShipState::Idle:
        default:
            return shipIdle.get();
    }
}

void GameWindow::render(double shipX, double shipY, const SpaceStation& station,
                        const std::vector<Bullet>& bullets, int frameCount, const std::string& status,
                        ShipState shipState)
{
    if (!hwnd || !backBufferDC) return;

    // Create graphics context on back buffer
    Gdiplus::Graphics graphics(backBufferDC);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    // Clear background
    Gdiplus::SolidBrush blackBrush(Gdiplus::Color(0, 0, 0));
    graphics.FillRectangle(&blackBrush, 0, 0, windowWidth, windowHeight);

    // Draw station (circle)
    Vector2D stationPos = station.getPosition();
    Gdiplus::SolidBrush stationBrush(Gdiplus::Color(255, 200, 0));
    graphics.FillEllipse(&stationBrush,
        (INT)(stationPos.getX() - 30), (INT)(stationPos.getY() - 30), 60, 60);

    // Draw station health indicator
    Gdiplus::Pen healthPen(Gdiplus::Color(255, 0, 0), 2);
    graphics.DrawRectangle(&healthPen, (INT)(stationPos.getX() - 35), (INT)(stationPos.getY() - 40), 70, 10);

    // Draw ship sprite
    Gdiplus::Image* shipImage = getShipImage(shipState);
    if (shipImage && shipImage->GetLastStatus() == Gdiplus::Ok) {
        graphics.DrawImage(shipImage, (INT)(shipX - 16), (INT)(shipY - 16), 32, 32);
    } else {
        // Fallback to circle if image fails to load
        Gdiplus::SolidBrush shipBrush(Gdiplus::Color(0, 255, 0));
        graphics.FillEllipse(&shipBrush, (INT)(shipX - 10), (INT)(shipY - 10), 20, 20);
    }

    // Draw bullets
    Gdiplus::SolidBrush bulletBrush(Gdiplus::Color(255, 100, 100));
    for (const auto& bullet : bullets) {
        Vector2D bPos = bullet.getPosition();
        graphics.FillEllipse(&bulletBrush, (INT)(bPos.getX() - 3), (INT)(bPos.getY() - 3), 6, 6);
    }

    // Draw UI text
    Gdiplus::Font font(L"Arial", 14);
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255));

    // Frame counter
    std::wostringstream frameText;
    frameText << L"Frame: " << frameCount;
    Gdiplus::PointF framePos(10.0f, 10.0f);
    graphics.DrawString(frameText.str().c_str(), -1, &font, framePos, &textBrush);

    // Ship position
    std::wostringstream shipText;
    shipText << L"Ship: (" << (int)shipX << ", " << (int)shipY << ")";
    Gdiplus::PointF shipTextPos(10.0f, 35.0f);
    graphics.DrawString(shipText.str().c_str(), -1, &font, shipTextPos, &textBrush);

    // Station position
    std::wostringstream stationText;
    stationText << L"Station: (" << (int)stationPos.getX() << ", " << (int)stationPos.getY() << ")";
    Gdiplus::PointF stationTextPos(10.0f, 60.0f);
    graphics.DrawString(stationText.str().c_str(), -1, &font, stationTextPos, &textBrush);

    // Status message
    size_t statusLen = status.length();
    std::wstring wideStatus(statusLen, L' ');
    std::copy(status.begin(), status.end(), wideStatus.begin());
    Gdiplus::PointF statusPos(10.0f, (float)(windowHeight - 30));
    graphics.DrawString(wideStatus.c_str(), -1, &font, statusPos, &textBrush);

    // Bullet count
    std::wostringstream bulletText;
    bulletText << L"Bullets: " << bullets.size();
    Gdiplus::PointF bulletPos((float)(windowWidth - 150), 10.0f);
    graphics.DrawString(bulletText.str().c_str(), -1, &font, bulletPos, &textBrush);

    // Blit back buffer to screen
    HDC screenDC = GetDC(hwnd);
    BitBlt(screenDC, 0, 0, windowWidth, windowHeight, backBufferDC, 0, 0, SRCCOPY);
    ReleaseDC(hwnd, screenDC);
}

void GameWindow::processMessages()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
