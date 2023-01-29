#include "pch.h"
#include "Input.h"

DirectInput::DirectInput()
    : mDirectInput(nullptr)
    , mKeyboardInput(nullptr)
    , mMouseInput(nullptr)
    , mMouseX(0)
    , mMouseY(0)
    , mScreenWidth(0)
    , mScreenHeight(0)
    , mControlState(0)
{
    ZeroMemory(&mMouseState, sizeof(mMouseState));
    ZeroMemory(&mKeyboardState, sizeof(mKeyboardState));
}

DirectInput::~DirectInput()
{
    ASSERT(mKeyboardInput == nullptr, "mKeyboardInput가 Unacquire되지 않았습니다.");
    ASSERT(mMouseInput == nullptr, "mMouseInput가 Unacquire되지 않았습니다");
}

HRESULT DirectInput::Initialize(HINSTANCE hInstance, HWND hwnd, int screenWidth, int screenHeight)
{
    HRESULT result = S_OK;

    result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&mDirectInput, nullptr);
    if (FAILED(result))
    {
        return E_FAIL;
    }

    result = mDirectInput->CreateDevice(GUID_SysKeyboard, &mKeyboardInput, nullptr);
    if (FAILED(result))
    {
        return E_FAIL;
    }

    mScreenWidth = screenWidth;
    mScreenHeight = screenHeight;
    //
    // keyboard
    //

    result = mKeyboardInput->SetDataFormat(&c_dfDIKeyboard);
    if (FAILED(result))
    {
        return E_FAIL;
    }

    result = mKeyboardInput->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
    if (FAILED(result))
    {
        return E_FAIL;
    }

    result = mKeyboardInput->Acquire();
    if (FAILED(result))
    {
        return E_FAIL;
    }

    ZeroMemory(mKeyboardState, sizeof(mKeyboardState));

    //
    // mouse
    //

    mDirectInput->CreateDevice(GUID_SysMouse, &mMouseInput, nullptr);
    if (FAILED(result))
    {
        return E_FAIL;
    }

    mMouseInput->SetDataFormat(&c_dfDIMouse);
    if (FAILED(result))
    {
        return E_FAIL;
    }

    mMouseInput->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);
    if (FAILED(result))
    {
        return E_FAIL;
    }

    result = mMouseInput->Acquire();
    if (FAILED(result))
    {
        return E_FAIL;
    }

    return result;
}

void DirectInput::Release()
{
    mKeyboardInput->Unacquire();
    mMouseInput->Unacquire();

    SAFETY_RELEASE(mKeyboardInput);
    SAFETY_RELEASE(mMouseInput);
}

void DirectInput::UpdateWindowSize(int newWidth, int newHeight)
{
    mScreenWidth = newWidth;
    mScreenHeight = newHeight;
}

HRESULT DirectInput::UpdateInput()
{
    //
    // keyboard
    //

    HRESULT result = mKeyboardInput->GetDeviceState(sizeof(mKeyboardState), &mKeyboardState);
    if (FAILED(result))
    {
        if ((result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED))
        {
            mKeyboardInput->Acquire();

        }
        else
        {
            return E_FAIL;
        }
    }

    //
    // mouse
    //

    result = mMouseInput->GetDeviceState(sizeof(mMouseState), &mMouseState);
    if (FAILED(result))
    {
        if ((result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED))
        {
            mMouseInput->Acquire();

        }
        else
        {
            return E_FAIL;
        }
    }
    mMouseX = mScreenWidth / 2;
    mMouseY = mScreenHeight / 2;

    mMouseX += mMouseState.lX;
    mMouseY += mMouseState.lY;

    mOriginalMouseX = mMouseState.lX;
    mOriginalMouseY = mMouseState.lY;

    // clamp
    if (mMouseX < 0)
    {
        mMouseX = 0;
    }
    if (mMouseY < 0)
    {
        mMouseY = 0;
    }

    if (mMouseX > mScreenWidth)
    {
        mMouseX = mScreenWidth;
    }
    if (mMouseY > mScreenHeight)
    {
        mMouseY = mScreenHeight;
    }

    return result;
}

void DirectInput::SetControlMode(uint32 flag)
{
    mControlState ^= flag;
}

void DirectInput::GetMousePosition(int& mouseX, int& mouseY) const
{
    mouseX = mMouseX;
    mouseY = mMouseY;
}

void DirectInput::GetMouseDeltaPosition(int& deltaX, int& deltaY) const
{
    deltaX = mOriginalMouseX;
    deltaY = mOriginalMouseY;
}

unsigned char* DirectInput::GetKeyboardPress()
{
    return mKeyboardState;
}

uint32 DirectInput::GetControlMode() const
{
    return mControlState;
}


/*
 *
 * MyInput Class
 *
 */

MyInput::MyInput()
    : mWnd(nullptr)
    , mScreenWidth(0)
    , mScreenHeight(0)
    , mMouseX(0)
    , mMouseY(0)
{
    ZeroMemory(&mKeyboardState, sizeof(mKeyboardState));
}

MyInput::~MyInput()
{
    Release();
}

HRESULT MyInput::Initialize(HWND* const hwnd, int screenWidth, int screenHeight)
{
    ASSERT(hwnd != nullptr, "hwnd가 nullptr입니다.");

    mWnd = &(*hwnd);
    mScreenWidth = screenWidth;
    mScreenHeight = screenHeight;
    return S_OK;
}

void MyInput::Release()
{
    mWnd = nullptr;
}

void MyInput::UpdateWindowSize(int newWidth, int newHeight)
{
    mScreenWidth = newWidth;
    mScreenHeight = newHeight;
}

bool MyInput::UpdateMouseInput(const MSG& msg)
{

    if (msg.message == WM_MOUSEMOVE)
    {
        POINT curPosition;
        GetCursorPos(&curPosition);

        POINT centerPosition;
        centerPosition.x = mScreenWidth / (float)2;
        centerPosition.y = mScreenHeight / (float)2;

        mMouseDeltaX = curPosition.x - centerPosition.x;
        mMouseDeltaY = curPosition.y - centerPosition.y;

        SetCursorPos(centerPosition.x, centerPosition.y);
        return true;
    }
    return false;
}

bool MyInput::UpdateKeyboardInput(const MSG& msg)
{
    if (msg.message == WM_KEYDOWN)
    {
        mKeyboardState[msg.wParam] = true;
        return true;
    }

    if (msg.message == WM_KEYUP)
    {
        mKeyboardState[msg.wParam] = false;
        return true;
    }
    return false;
}

void MyInput::SetControlMode(uint32 flag)
{
    mControlState ^= flag;
}

void MyInput::GetMousePosition(int& mouseX, int& mouseY) const
{
    POINT curPosition;
    GetCursorPos(&curPosition);
    mouseX = curPosition.x;
    mouseY = curPosition.y;
}

void MyInput::GetMouseDeltaPosition(int& deltaX, int& deltaY) const
{
    deltaX = mMouseDeltaX;
    deltaY = mMouseDeltaY;
}

void MyInput::GetKeyboardPressed(bool* keyboardState, int* size) const
{
    ASSERT(keyboardState != nullptr, "keyboardState가 nullptr입니다.");

    enum { KEYBOARD_STATE_ARR_SIZE = 256 };
    *size = KEYBOARD_STATE_ARR_SIZE;
    memcpy(keyboardState, mKeyboardState, sizeof(bool) * KEYBOARD_STATE_ARR_SIZE);
}

uint32 MyInput::GetControlMode() const
{
    return mControlState;
}
