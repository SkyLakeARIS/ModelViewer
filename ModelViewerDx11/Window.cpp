#include "Window.h"
#include "ModelViewerDx11.h"

ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

#define MAX_LOADSTRING 100

WCHAR       szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR       szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.


Window::Window(HINSTANCE hInstance, int32_t nCmdShow, int16_t width, int16_t height)
    : mHandleWindow(nullptr)
    , mHandleInstance(hInstance)
    , mCmdShow(nCmdShow)
    , mAppTitleName{}
    , mWindowClassName{}
    , mWindowWidth(width)
    , mWindowHeight(height)
{
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MODELVIEWERDX11, szWindowClass, MAX_LOADSTRING);
}

Window::~Window()
{
    
}

HWND Window::MakeWindow()
{
    mHandleWindow = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        0, 0, mWindowWidth, mWindowHeight, nullptr, nullptr, mHandleInstance, nullptr);
    return mHandleWindow;
}

void Window::DisplayWindow()
{
    ShowWindow(mHandleWindow, mCmdShow);
}

void Window::RefreshWindow()
{
    UpdateWindow(mHandleWindow);
}

ATOM Window::RegisterWindowClass()
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
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
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
