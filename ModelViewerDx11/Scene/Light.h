#pragma once
#include "../Renderer/Resources/Mesh.h"

namespace scene
{
    class Camera;

    // MEMO: 만들어놓고 XMVECTOR 비율이 더 높으면 XMFLOAT3->XMVECTOR로
    class Light final // working like directional light
    {
    public:
        Light(XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 color, float nearPlane, float farPlane);
        ~Light();

        void SubmitDebugCommand();

        void Update();

        XMFLOAT4 GetDirection() const;
        XMFLOAT3 GetPosition() const;
        XMFLOAT4 GetColor() const;
        XMMATRIX GetViewProjMatrix() const;

    private:

        XMFLOAT3 mPosition;
        XMFLOAT3 mDirection;
        XMFLOAT3 mColor;

        // for shadow-map
        XMMATRIX mMatView;
        XMMATRIX mMatProj;
        XMMATRIX mMatViewProj;

        std::vector<XMFLOAT3> mLines;
        renderer::Mesh mMeshDebug;

        float mNearPlane;
        float mFarPlane;
    };
}
