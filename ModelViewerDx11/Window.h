#pragma once
#include "Windows.h"
#include "Util/Type.h"

class Window
{
public:
    Window(HINSTANCE hInstance);
    ~Window() = default;

    HWND MakeWindow(int16_t windowWidth, int16_t windowHeight);
    void DisplayWindow(int32_t nCmdShow) const;
    void RefreshWindow() const;
    void RegisterWindowClass() const;

    bool ProcessMessages();

    HWND GetHandle() const;
    const char* GetAppNameA() const;
public:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public:
    static constexpr int32_t MAX_WINDOW_NAME_LENGTH = 256;
private:
    HWND mHandleWindow;
    HINSTANCE mHandleInstance;

    WCHAR mAppTitleNameW[MAX_WINDOW_NAME_LENGTH];
    char mAppTitleNameA[MAX_WINDOW_NAME_LENGTH * 2];
    WCHAR mWindowClassName[MAX_WINDOW_NAME_LENGTH];

};
