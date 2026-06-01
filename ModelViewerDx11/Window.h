#pragma once
#include "framework.h"

class Window
{
public:
    Window(HINSTANCE hInstance, int16_t width, int16_t height);
    ~Window() = default;

    HWND MakeWindow();
    void DisplayWindow(int32_t nCmdShow) const;
    void RefreshWindow() const;
    void RegisterWindowClass() const;

    bool ProcessMessages();

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public:
    static constexpr int32_t MAX_WINDOW_NAME_LENGTH = 256;
private:
    HWND mHandleWindow;
    HINSTANCE mHandleInstance;

    WCHAR mAppTitleName[MAX_WINDOW_NAME_LENGTH];
    WCHAR mWindowClassName[MAX_WINDOW_NAME_LENGTH];

    int16_t mWindowWidth;
    int16_t mWindowHeight;
};
