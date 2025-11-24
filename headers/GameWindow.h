#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <memory>
#include "SpaceShip.h"
#include "SpaceStation.h"
#include "Bullet.h"

#pragma comment(lib, "gdiplus.lib")

enum class ShipState {
    Idle,
    Boost,
    Left,
    Right
};

class GameWindow {
private:
    HWND hwnd;
    int windowWidth;
    int windowHeight;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    bool isRunning;

    // Double buffering
    HDC backBufferDC;
    HBITMAP backBufferBitmap;
    HBITMAP oldBitmap;

    // Ship images
    std::unique_ptr<Gdiplus::Image> shipIdle;
    std::unique_ptr<Gdiplus::Image> shipBoost;
    std::unique_ptr<Gdiplus::Image> shipLeft;
    std::unique_ptr<Gdiplus::Image> shipRight;
    ShipState lastShipState;

    // Window procedure
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    // Load images
    bool loadImages();
    Gdiplus::Image* getShipImage(ShipState state);

public:
    GameWindow(int width, int height, const char* title);
    ~GameWindow();

    bool initialize();
    void render(double shipX, double shipY, const SpaceStation& station,
                const std::vector<Bullet>& bullets, int frameCount, const std::string& status,
                ShipState shipState);
    bool isOpen() const { return isRunning; }
    void processMessages();
    void close() { isRunning = false; }
};
