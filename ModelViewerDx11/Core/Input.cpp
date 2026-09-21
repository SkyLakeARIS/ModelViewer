#include "Input.h"
#include "../Util/Macro.h"

namespace core
{
    DirectInput::DirectInput(HINSTANCE hInstance, HWND hwnd, int screenWidth, int screenHeight)
        : mDirectInput(nullptr)
        , mKeyboardInput(nullptr)
        , mhInstance(hInstance)
        , mhWnd(hwnd)
        , mMouseInput(nullptr)
        , mMouseX(0)
        , mMouseY(0)
        , mScreenWidth(screenWidth)
        , mScreenHeight(screenHeight)
        , mControlState(0)
        , mMouseInputFlag(0)
    {
        ZeroMemory(&mMouseState, sizeof(mMouseState));
        ZeroMemory(&mKeyboardState, sizeof(mKeyboardState));
    }

    DirectInput::~DirectInput()
    {
        ASSERT(mKeyboardInput == nullptr, "mKeyboardInput가 Unacquire되지 않았습니다.");
        ASSERT(mMouseInput == nullptr, "mMouseInput가 Unacquire되지 않았습니다");
    }

    HRESULT DirectInput::Initialize()
    {
        HRESULT result = S_OK;

        result = DirectInput8Create(mhInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&mDirectInput, nullptr);
        if (FAILED(result))
        {
            return E_FAIL;
        }

        result = mDirectInput->CreateDevice(GUID_SysKeyboard, &mKeyboardInput, nullptr);
        if (FAILED(result))
        {
            return E_FAIL;
        }

        //
        // keyboard
        //

        result = mKeyboardInput->SetDataFormat(&c_dfDIKeyboard);
        if (FAILED(result))
        {
            return E_FAIL;
        }

        result = mKeyboardInput->SetCooperativeLevel(mhWnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
        if (FAILED(result))
        {
            return E_FAIL;
        }

        mKeyboardInput->Acquire();


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

        mMouseInputFlag = DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND;
        mMouseInput->SetCooperativeLevel(mhWnd, mMouseInputFlag);
        if (FAILED(result))
        {
            return E_FAIL;
        }

        mMouseInput->Acquire();
        //if (FAILED(result))
        //{
        //    return E_FAIL;
        //}

        return result;
    }

    void DirectInput::Release()
    {
        if(mKeyboardInput)
        {
            mKeyboardInput->Unacquire();
        }

        if(mMouseInput)
        {
            mMouseInput->Unacquire();
        }

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
            if (result | (DIERR_INPUTLOST | DIERR_NOTACQUIRED))
            {
                mKeyboardInput->Acquire();

            }
            else
            {
                return E_FAIL;
            }
        }

        uint32_t flag = 0;
        if (mKeyboardState[DIK_LALT])
        {
            flag = DISCL_NOWINKEY | DISCL_NONEXCLUSIVE | DISCL_FOREGROUND;
        }
        else
        {
            flag = DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND;
        }

        if (mMouseInputFlag != flag)
        {
            mMouseInput->Unacquire();

            mMouseInputFlag = flag;
            mMouseInput->SetCooperativeLevel(mhWnd, mMouseInputFlag);
            if (FAILED(result))
            {
                return E_FAIL;
            }

            mMouseInput->Acquire();
        }

        //
        // mouse
        //
     /*   result = mMouseInput->GetDeviceState(sizeof(mMouseState), &mMouseState);
        if (FAILED(result))
        {
            if(result | (DIERR_INPUTLOST | DIERR_NOTACQUIRED))
            {
                mMouseInput->Acquire();

            }
            else
            {
                return E_FAIL;
            }
        }*/

        //if(mKeyboardState[DIK_LALT])
        //{
        //    mOriginalMouseX = 0;
        //    mOriginalMouseY = 0;
        //    // ALT key is cursor control mode
        //    return S_OK;
        //}

        //mMouseX = mScreenWidth / 2;
        //mMouseY = mScreenHeight / 2;

        //mMouseX += mMouseState.lX;
        //mMouseY += mMouseState.lY;

        //mOriginalMouseX = mMouseState.lX;
        //mOriginalMouseY = mMouseState.lY;

        //// clamp
        //if(mMouseX < 0)
        //{
        //    mMouseX = 0;
        //}
        //if (mMouseY < 0)
        //{
        //    mMouseY = 0;
        //}

        //if (mMouseX > mScreenWidth)
        //{
        //    mMouseX = mScreenWidth;
        //}
        //if (mMouseY > mScreenHeight)
        //{
        //    mMouseY = mScreenHeight;
        //}

        return result;
    }

    void DirectInput::SetControlMode(uint32_t flag)
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
        if (mKeyboardState[DIK_LALT])
        {
            deltaX = 0;
            deltaY = 0;
            return;
        }

        HRESULT result = mMouseInput->GetDeviceState(sizeof(mMouseState), (LPVOID)&mMouseState);
        if (FAILED(result))
        {
            if (result | (DIERR_INPUTLOST | DIERR_NOTACQUIRED))
            {
                mMouseInput->Acquire();

            }
            else
            {
                return;
            }
        }
        deltaX = mMouseState.lX;
        deltaY = mMouseState.lY;
    }

    const unsigned char* DirectInput::GetKeyboardPress() const
    {
        return mKeyboardState;
    }

    uint32_t DirectInput::GetControlMode() const
    {
        return mControlState;
    }
}
