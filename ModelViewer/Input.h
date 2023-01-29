#pragma once
#include "framework.h"

/*
 * DirectInput 사용 클래스
 */
enum class eControlFlags : uint32
{
    KEYBOARD_MOVEMENT_MODE = 1, // 1 == keyboard control, 0 == mouse control
};
class DirectInput final
{

public:

    DirectInput();
    virtual ~DirectInput();

    HRESULT     Initialize(HINSTANCE hInstance, HWND hwnd, int screenWidth, int screenHeight);
    void        Release();

    void        UpdateWindowSize(int newWidth, int newHeight);

    HRESULT     UpdateInput();

    void        SetControlMode(uint32 flag);

    void                GetMousePosition(int& mouseX, int& mouseY) const;
    void                GetMouseDeltaPosition(int& deltaX, int& deltaY) const;
    unsigned char*      GetKeyboardPress();
    uint32              GetControlMode() const;

private:
    IDirectInput8*          mDirectInput;
    IDirectInputDevice8*    mKeyboardInput;
    IDirectInputDevice8*    mMouseInput;

    unsigned char           mKeyboardState[256];
    DIMOUSESTATE            mMouseState;

    int                     mMouseX;
    int                     mMouseY;
    int                     mOriginalMouseX;
    int                     mOriginalMouseY;

    int                     mScreenWidth;
    int                     mScreenHeight;

    UINT32                  mControlState;
};


/*
 * 직접 만든 keyboard/mouse Input 클래스
 *
 * 현재는 DirectInput과 다른점이 안보이나, 프로그램이 커지기 시작해야
 * 성능 문제가 보이지 않을까 예상.
 */
class MyInput final
{
public:
    MyInput();
    ~MyInput();

    HRESULT     Initialize(HWND* const hwnd, int screenWidth, int screenHeight);
    void        Release();

    void        UpdateWindowSize(int newWidth, int newHeight);

    bool        UpdateMouseInput(const MSG& msg);
    bool        UpdateKeyboardInput(const MSG& msg);

    void        SetControlMode(uint32 flag);

    void        GetMousePosition(int& mouseX, int& mouseY) const;
    void        GetMouseDeltaPosition(int& deltaX, int& deltaY) const;
    void        GetKeyboardPressed(bool* keyboardState, int* size) const;
    uint32      GetControlMode() const;

private:
    HWND* mWnd;
    bool                    mKeyboardState[256];

    int                     mMouseX;
    int                     mMouseY;
    int                     mMouseDeltaX;
    int                     mMouseDeltaY;

    int                     mScreenWidth;
    int                     mScreenHeight;

    UINT32                  mControlState;
};