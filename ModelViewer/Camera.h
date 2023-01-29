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
    XMFLOAT2    mAnglesRad;         // (������) ���鿡���� ��ġ�� ����ϱ� ���� ��, x == pi, y == theta
    XMVECTOR    mPositionInSphere;  // (������) ���鿡���� ��ǥ

    float       mRadiusOfSphere;    // ������, ��ü ũ��

    XMVECTOR    mvEye;              // ���� ��ǥ���� ���� ���� ī�޶� ��ġ
    XMVECTOR    mvLookAtCenter;     // ��ü�� �߽�(�˵� ī�޶��� ����)
    XMVECTOR    mvUp;

    XMVECTOR    mvForward;          // �ʿ����.
    XMVECTOR    mvRight;            // �ʿ����.


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