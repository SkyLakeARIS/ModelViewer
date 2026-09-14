#pragma once
#include "InputPrerequisites.h"
#include "../Util/Type.h"

namespace core
{
    /*
     * DirectInput 사용 클래스
     */
    enum class eControlFlags : uint32_t
    {
        KEYBOARD_MOVEMENT_MODE = 1, // 1 == keyboard control, 0 == mouse control
    };
    class DirectInput final
    {

    public:

        DirectInput(HINSTANCE hInstance, HWND hwnd, int screenWidth, int screenHeight);
        ~DirectInput();

        HRESULT     Initialize();
        void        Release();

        void        UpdateWindowSize(int newWidth, int newHeight);

        HRESULT     UpdateInput();

        void        SetControlMode(uint32_t flag);

        void                GetMousePosition(int& mouseX, int& mouseY) const;
        void                GetMouseDeltaPosition(int& deltaX, int& deltaY) const;
        const unsigned char* GetKeyboardPress() const;
        uint32_t              GetControlMode() const;

    private:
        IDirectInput8* mDirectInput;
        IDirectInputDevice8* mKeyboardInput;
        IDirectInputDevice8* mMouseInput;

        HINSTANCE               mhInstance;
        HWND                    mhWnd;

        uint32_t                  mMouseInputFlag;

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
}
