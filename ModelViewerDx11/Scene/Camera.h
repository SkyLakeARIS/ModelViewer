#pragma once
#include "../framework.h"

namespace scene
{
    // TODO: 그림자 효과 추가하면 FPS로 동작하는 Camera를 분리하는것이 Light클래스 관리에 도움이 될 듯 함.
    class Camera final
    {

    public:
        Camera(XMVECTOR vEye, XMVECTOR vLookAt, XMVECTOR vUp, int16_t windowWidth, int16_t windowHeight);
        virtual ~Camera();

        void RotateAxis(float yawRad, float pitchRad);

        void AddRadiusSphere(float scaleFactor);
        void AddHeight(float height);

        void ChangeFocus(XMFLOAT3 newFocus);

        float GetFov() const;
        float GetAspectRatio() const;
        XMFLOAT3 GetCameraPositionFloat() const;
        XMVECTOR GetCameraPositionVector() const;

        inline XMMATRIX GetViewMatrix() const;
        inline XMMATRIX GetViewProjectionMatrix() const;
        inline XMMATRIX GetProjectionMatrix() const;
    private:

        void calcCameraPosition();

        void makeViewMatrix();
        void makeProjectionMatrix();

    private:

        XMFLOAT2    mAnglesRad;         // (가상의) 구면에서의 위치를 계산하기 위한 각, x == pi, y == theta
        XMVECTOR    mPositionInSphere;  // (가상의) 구면에서의 좌표

        float       mRadiusOfSphere;    // 반지름, 구체 크기

        XMVECTOR    mvEye;              // 구면 좌표에서 계산된 실제 카메라 위치
        XMVECTOR    mvLookAtCenter;     // 구체의 중심(궤도 카메라의 초점)
        XMVECTOR    mvUp;

        XMVECTOR    mvForward;          // 필요없음.
        XMVECTOR    mvRight;            // 필요없음.

        float       mFov;
        uint32      mScreenWidth;
        uint32      mScreenHeight;

        XMMATRIX    mMatView;
        XMMATRIX    mMatProjection;
        XMMATRIX    mMatViewProjection;

        //  ID3D11Buffer* mCbViewProjection;    // 모델에서 가지게 될 듯.
    };

    inline XMMATRIX Camera::GetViewMatrix() const
    {
        return mMatView;
    }

    inline XMMATRIX Camera::GetProjectionMatrix() const
    {
        return mMatProjection;
    }

    inline XMMATRIX Camera::GetViewProjectionMatrix() const
    {
        return mMatViewProjection;
    }
}
