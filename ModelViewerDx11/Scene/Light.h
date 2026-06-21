#pragma once
#include "../framework.h"
#include "../Renderer/Resources/ModelData.h"

namespace renderer
{
    class Renderer;
    class TextureManager;
    class Plane;
}

namespace scene
{
    class Camera;

    // MEMO: 만들어놓고 XMVECTOR 비율이 더 높으면 XMFLOAT3->XMVECTOR로
    class Light final // working like directional light
    {
        enum eCascadeLevel
        {
            Level_4 = 5 // near - 1 - 2- 3- 4 - far
        };
    public:
        Light(XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 color, Camera* camera, float nearPlane, float farPlane, renderer::Renderer& renderer);
        ~Light();

        void Initialize(renderer::TextureManager* const texManager, renderer::Renderer& renderer);
        void Update(renderer::Renderer& renderer);
        void Draw(renderer::Renderer& renderer);
        // TODO: 별도 함수보다 GUI 추가되면 Debug모드 추가하여 on/off 방식으로 처리
        void DrawDebug(renderer::Renderer& renderer);

        void SetupCascade(renderer::Renderer& renderer);

        XMFLOAT4 GetDirection() const;
        XMFLOAT4 GetPosition() const;
        XMFLOAT4 GetColor() const;
        const XMMATRIX* const GetViewProjMatrix() const;

    private:

        void updateMatrices(renderer::Renderer& renderer);
        void updateLightPropertyCB(renderer::Renderer& renderer);


        void getPointsFromMatrix(XMMATRIX* matView, float nearPlane, float farPlane, XMMATRIX* const outMatLightView, XMMATRIX* const outMatLightProj, renderer::Renderer& renderer);
    private:

        XMFLOAT3 mPosition;
        XMFLOAT3 mDirection;
        XMFLOAT3 mColor;

        // for shadow-map
        XMMATRIX mMatView;
        XMMATRIX mMatProj;
        XMMATRIX mMatViewProj;

        renderer::Plane* mMesh;
        XMMATRIX mMatWorld;
        HashID mBlendHash;

        HashID mIconTexHash;
        std::vector<XMFLOAT3> mLines;
        renderer::Mesh mMeshDebug;

        float mNearPlane;
        float mFarPlane;
        Camera* mCamera;
        XMMATRIX mMatLightViews[eCascadeLevel::Level_4];
        XMMATRIX mMatLightProjs[eCascadeLevel::Level_4];
        float mCascadePlaneDistances[eCascadeLevel::Level_4 + 1];
    };
}
