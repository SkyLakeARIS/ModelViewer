#pragma once
#include "framework.h"

class Window
{
public:
    Window(HINSTANCE hInstance, int32_t nCmdShow, int16_t width, int16_t height);
    ~Window();

    HWND MakeWindow();
    void DisplayWindow();
    void RefreshWindow();
    ATOM RegisterWindowClass();

    bool ProcessMessages();

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:

public:
    static constexpr int32_t MAX_WINDOW_NAME_LENGTH = 256;
private:
    HWND mHandleWindow;
    HINSTANCE mHandleInstance;
    int32_t mCmdShow;

    WCHAR mAppTitleName[MAX_WINDOW_NAME_LENGTH];
    WCHAR mWindowClassName[MAX_WINDOW_NAME_LENGTH];

    int16_t mWindowWidth;
    int16_t mWindowHeight;
};
