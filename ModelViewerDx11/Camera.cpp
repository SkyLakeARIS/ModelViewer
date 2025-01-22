#include "Camera.h"
#include "Renderer.h"
#include "cmath"

Camera::Camera(XMVECTOR vEye, XMVECTOR vLookAt, XMVECTOR vUp)
    : mRadiusOfSphere(2.0f)
    , mvEye(vEye)
    , mvLookAtCenter(vLookAt)
    , mvUp(vUp)
{
    Renderer::GetInstance()->GetWindowSize(mScreenWidth, mScreenHeight);

    /*
     * 직교좌표에서 구면좌표로 역계산.
     * 카메라 위치와 가상의 구면 위치와 동기화하기 위함.
     * (반지름) r이 1인 단위 구체로 생각하고 계산한다.
     */
    XMFLOAT3 eyePos;
    XMStoreFloat3(&eyePos, vEye);
    eyePos.x = XMConvertToRadians(eyePos.x);
    eyePos.y = XMConvertToRadians(eyePos.y);
    eyePos.z = XMConvertToRadians(eyePos.z);

    mAnglesRad.x = atan(eyePos.x / -eyePos.z);
    mAnglesRad.y = acos(eyePos.y);

    mFov = XMConvertToRadians(60.0f);

    makeViewMatrix();
    makeProjectionMatrix();
}

Camera::~Camera()
{
   // SAFETY_RELEASE(mCbViewProjection);

}

void Camera::RotateAxis(float yawRad, float pitchRad)
{
    mAnglesRad.x += yawRad;         // pi, yaw
    mAnglesRad.y += pitchRad;       // theta, pitch

    // pitch만 클램프, yaw는 순환
    // pitch limit는 -85.0~85.0
    constexpr float PITCH_LIMIT = 0.261799f; // 15 degrees
    if(mAnglesRad.x >= XM_2PI)
    {
        mAnglesRad.x -= XM_2PI;
    }
    else if (mAnglesRad.x < 0.0f)
    {
        mAnglesRad.x += XM_2PI;
    }

    if (mAnglesRad.y > (XM_PI - PITCH_LIMIT))
    {
        mAnglesRad.y = XM_PI - PITCH_LIMIT;
    }
    else if (mAnglesRad.y < PITCH_LIMIT)
    {
        mAnglesRad.y = PITCH_LIMIT;
    }

    // (반지름) r이 1인 단위 구체로 생각하고 계산 후, radius만큼 거리를 조정한다.
    calcCameraPosition();

    makeViewMatrix();
}

void Camera::AddRadiusSphere(float scaleFactor)
{
    constexpr float MAX_RADIUS = 10.0f;
    constexpr float MIN_RADIUS = 0.1f;

    mRadiusOfSphere += scaleFactor *2.0f;

    if(mRadiusOfSphere > MAX_RADIUS)
    {
        mRadiusOfSphere = MAX_RADIUS;
    }
    else if(mRadiusOfSphere < MIN_RADIUS)
    {
        mRadiusOfSphere = MIN_RADIUS;
    }

    // 변경된 거리를 적용한다.
    calcCameraPosition();

    makeViewMatrix();
}

void Camera::AddHeight(float height)
{
    XMFLOAT3 eye = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT3 lookAt = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMStoreFloat3(&eye, mvEye);
    XMStoreFloat3(&lookAt, mvLookAtCenter);

    // TODO 현재 타겟으로 하는 물체의 최대/최소 높이를 구해서 제한 걸어야 함.
    eye.y += height;
    lookAt.y += height;

    mvEye = XMLoadFloat3(&eye);
    mvLookAtCenter = XMLoadFloat3(&lookAt);

    calcCameraPosition();

    makeViewMatrix();
}

void Camera::ChangeFocus(XMFLOAT3 newFocus)
{
    mvLookAtCenter = XMLoadFloat3(&newFocus);

    // 중심이 변경되었으므로 카메라 위치를 다시 계산한다.
    calcCameraPosition();

    makeViewMatrix();
}

float Camera::GetFov() const
{
    return mFov;
}

float Camera::GetAspectRatio() const
{
    return static_cast<float>(mScreenWidth) / static_cast<float>(mScreenHeight);
}

XMVECTOR Camera::GetCameraPositionVector() const
{
    return mvEye;
}

XMFLOAT3 Camera::GetCameraPositionFloat() const
{
    XMFLOAT3 position;
    XMStoreFloat3(&position, mvEye);
    return position;
}

void Camera::calcCameraPosition()
{
    // (반지름) r이 1인 단위 구체로 생각하고 계산 후, radius만큼 거리를 조정한다.
    XMFLOAT3 positionInSphere;
    positionInSphere.x = sin(mAnglesRad.y) * sin(mAnglesRad.x);
    positionInSphere.y = cos(mAnglesRad.y);
    positionInSphere.z = -(sin(mAnglesRad.y) * cos(mAnglesRad.x));

    const XMVECTOR newPositionInOrbit = XMVectorScale(XMLoadFloat3(&positionInSphere), mRadiusOfSphere);
    // 가상의 구체를 통해 얻은 좌표로 실제 위치인 mvLookAtCenter를 중심으로 하는 궤도로 이동
    mvEye = XMVectorAdd(newPositionInOrbit, mvLookAtCenter);

    XMFLOAT3 position;
    XMStoreFloat3(&position, mvEye);
    Renderer::CbCameraPosition cbCameraPos = {};
    cbCameraPos.Float3 = position;
    Renderer::GetInstance()->UpdateCB(Renderer::eCbType::CbCameraPosition, &cbCameraPos);
}

void Camera::makeViewMatrix()
{
    mMatView = XMMatrixLookAtLH(mvEye, mvLookAtCenter, mvUp);
    mMatViewProjection = mMatView * mMatProjection;
}

void Camera::makeProjectionMatrix()
{
    mMatProjection = XMMatrixPerspectiveFovLH(mFov, static_cast<float>(mScreenWidth) / static_cast<float>(mScreenHeight), 0.1f, 500.0f);
    mMatViewProjection = mMatView * mMatProjection;
}