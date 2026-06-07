#include "Window.h"
#include "ModelViewerDx11.h"

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

Window::Window(HINSTANCE hInstance)
    : mHandleWindow(nullptr)
    , mHandleInstance(hInstance)
    , mAppTitleName{}
    , mWindowClassName{}
{
    LoadStringW(mHandleInstance, IDS_APP_TITLE, mAppTitleName, MAX_WINDOW_NAME_LENGTH);
    LoadStringW(mHandleInstance, IDC_MODELVIEWERDX11, mWindowClassName, MAX_WINDOW_NAME_LENGTH);
}

HWND Window::MakeWindow(int16_t windowWidth, int16_t windowHeight)
{
    mHandleWindow = CreateWindowW(mWindowClassName, mAppTitleName, WS_OVERLAPPEDWINDOW,
        0, 0, windowWidth, windowHeight, nullptr, nullptr, mHandleInstance, nullptr);
    return mHandleWindow;
}

void Window::DisplayWindow(int32_t nCmdShow) const
{
    ShowWindow(mHandleWindow, nCmdShow);
}

void Window::RefreshWindow() const
{
    UpdateWindow(mHandleWindow);
}

void Window::RegisterWindowClass() const
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Window::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = mHandleInstance;
    wcex.hIcon = LoadIcon(mHandleInstance, MAKEINTRESOURCE(IDI_MODELVIEWERDX11));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MODELVIEWERDX11);
    wcex.lpszClassName = mWindowClassName;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassExW(&wcex);
}

bool Window::ProcessMessages()
{
    MSG message = {};
    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE | PM_NOYIELD))
    {
        if (message.message == WM_QUIT)
        {
            return true;
        }
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return false;
}

HWND Window::GetHandle() const
{
    return mHandleWindow;
}

LRESULT Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;

}
