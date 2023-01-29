#pragma once
#include "framework.h"

class Camera final
{
public:
    Camera(XMVECTOR vEye, XMVECTOR vLookAt, XMVECTOR vUp);
    Camera(XMFLOAT3 focus);
    virtual ~Camera();

    void            RotateAxis(float yawRad, float pitchRad);

    void            AddRadiusSphere(float scaleFactor);

    void            ChangeFocus(XMFLOAT3 newFocus);

    inline XMMATRIX GetViewMatrix() const;
    inline XMMATRIX GetProjectionMatrix();
private:

    void calcCameraPosition();

    void makeViewMatrix();

private:
    XMFLOAT2    mAnglesRad;         // (가상의) 구면에서의 위치를 계산하기 위한 각, x == pi, y == theta
    XMVECTOR    mPositionInSphere;  // (가상의) 구면에서의 좌표

    float       mRadiusOfSphere;    // 반지름, 구체 크기

    XMVECTOR    mvEye;              // 구면 좌표에서 계산된 실제 카메라 위치
    XMVECTOR    mvLookAtCenter;     // 구체의 중심(궤도 카메라의 초점)
    XMVECTOR    mvUp;

    XMVECTOR    mvForward;          // 필요없음.
    XMVECTOR    mvRight;            // 필요없음.


    XMMATRIX    mMatView;
    XMMATRIX    mMatProjection;

};

inline XMMATRIX Camera::GetViewMatrix() const
{
    return mMatView;
}

inline XMMATRIX Camera::GetProjectionMatrix()
{
    INT32 width = 1024;
    INT32 height = 720;
    mMatProjection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.001f, 1000.0f);
    return mMatProjection;
}