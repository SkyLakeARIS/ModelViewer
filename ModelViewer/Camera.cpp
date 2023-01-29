#include "pch.h"
#include "Camera.h"
#include "cmath"

Camera::Camera(XMVECTOR vEye, XMVECTOR vLookAt, XMVECTOR vUp)
    : mvEye(vEye)
    , mvLookAtCenter(vLookAt)
    , mvUp(vUp)
    , mRadiusOfSphere(0.0f)
{

    /*
     * ������ǥ���� ������ǥ�� �����.
     * ī�޶� ��ġ�� ������ ���� ��ġ�� ����ȭ�ϱ� ����.
     * (������) r�� 1�� ���� ��ü�� �����ϰ� ����Ѵ�.
     */
    XMFLOAT3 eyePos;
    XMStoreFloat3(&eyePos, vEye);
    eyePos.x = XMConvertToRadians(eyePos.x);
    eyePos.y = XMConvertToRadians(eyePos.y);
    eyePos.z = XMConvertToRadians(eyePos.z);

    mAnglesRad.x = atan(eyePos.x / -eyePos.z);
    mAnglesRad.y = acos(eyePos.y);

    XMFLOAT3 distance;
    XMStoreFloat3(&distance, XMVectorSubtract(mvEye, mvLookAtCenter));
    mRadiusOfSphere = sqrt(distance.x * distance.x + distance.y * distance.y + distance.z * distance.z);

}

//// lookat�� ��ġ�� setter�� �����Ƿ� eye, up�� ���޹��� �ʿ䰡 ���� ������
//// �ش� �����ڸ� �̿��ϵ��� �� �� �ְ� ����� ��
//Camera::Camera(XMFLOAT3 lookAt)
//    : mRadiusOfSphere(10.0f)
//{
//    mvLookAtCenter = XMLoadFloat3(&lookAt);
//
//    XMFLOAT3 eyePos = lookAt;
//    eyePos.z -= 10.0f;
//
//    mvEye = XMLoadFloat3(&eyePos);
//    /*
//     * ������ǥ���� ������ǥ�� �����.
//     * ī�޶� ��ġ�� ������ ���� ��ġ�� ����ȭ�ϱ� ����.
//     * (������) r�� 1�� ���� ��ü�� �����ϰ� ����Ѵ�.
//     */
//    XMStoreFloat3(&eyePos, mvEye);
//    eyePos.x = XMConvertToRadians(eyePos.x);
//    eyePos.y = XMConvertToRadians(eyePos.y);
//    eyePos.z = XMConvertToRadians(eyePos.z);
//
//    mAnglesRad.x = atan(eyePos.x / -eyePos.z);
//    mAnglesRad.y = acos(eyePos.y/ mRadiusOfSphere);
//
//    /*
//     *  up ���� ���
//     */
//    XMVECTOR vForward = XMVectorSubtract(mvLookAtCenter, mvEye);
//    XMVECTOR vRight = XMVectorSet(lookAt.x, 0.0f, 0.0f, 0.0f);
//    mvUp = XMVector3Cross(vForward, vRight);
//
//}

Camera::~Camera()
{

}


void Camera::RotateAxis(float yawRad, float pitchRad)
{
    mAnglesRad.x += yawRad;         // pi, yaw
    mAnglesRad.y += pitchRad;       // theta, pitch

    // pitch�� Ŭ����, yaw�� ��ȯ
    // pitch limit�� -85.0~85.0
    constexpr float PITCH_LIMIT = 0.261799; // 15 degree
    if (mAnglesRad.x >= XM_2PI)
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

    // (������) r�� 1�� ���� ��ü�� �����ϰ� ��� ��, radius��ŭ �Ÿ��� �����Ѵ�.
    calcCameraPosition();

    makeViewMatrix();
}

void Camera::AddRadiusSphere(float scaleFactor)
{
    constexpr float MAX_RADIUS = 80.0f;
    constexpr float MIN_RADIUS = 1.0f;

    mRadiusOfSphere += scaleFactor;

    if (mRadiusOfSphere > MAX_RADIUS)
    {
        mRadiusOfSphere = MAX_RADIUS;
    }
    else if (mRadiusOfSphere < MIN_RADIUS)
    {
        mRadiusOfSphere = MIN_RADIUS;
    }

    // ����� �Ÿ��� �����Ѵ�.
    calcCameraPosition();

    makeViewMatrix();
}

void Camera::ChangeFocus(XMFLOAT3 newFocus)
{
    mvLookAtCenter = XMLoadFloat3(&newFocus);

    // �߽��� ����Ǿ����Ƿ� ī�޶� ��ġ�� �ٽ� ����Ѵ�.
    calcCameraPosition();

    makeViewMatrix();
}

void Camera::calcCameraPosition()
{
    // (������) r�� 1�� ���� ��ü�� �����ϰ� ��� ��, radius��ŭ �Ÿ��� �����Ѵ�.
    XMFLOAT3 positionInSphere;
    positionInSphere.x = sin(mAnglesRad.y) * sin(mAnglesRad.x);
    positionInSphere.y = cos(mAnglesRad.y);
    positionInSphere.z = -(sin(mAnglesRad.y) * cos(mAnglesRad.x));

    const XMVECTOR newPositionInOrbit = XMVectorScale(XMLoadFloat3(&positionInSphere), mRadiusOfSphere);
    // ������ ��ü�� ���� ���� ��ǥ�� ���� ��ġ�� mvLookAtCenter�� �߽����� �ϴ� �˵��� �̵�
    mvEye = XMVectorAdd(newPositionInOrbit, mvLookAtCenter);
}

void Camera::makeViewMatrix()
{
    mMatView = XMMatrixLookAtLH(mvEye, mvLookAtCenter, mvUp);
}
