#pragma once
#include "../framework.h"
#include "../Renderer/Resources/ModelData.h"

namespace renderer
{
    class Renderer;
}

namespace scene
{
    class Camera;

    class Sky
    {
    public:

        Sky(renderer::Renderer& renderer, Camera& camera);
        ~Sky();

        HRESULT Initialize(uint32 latLines, uint32 lonLines);

        void Draw();
        void Update();
    private:

        HRESULT createSphere(uint32 latLines, uint32 lonLines);

    private:

        renderer::Renderer* mRenderer;
        Camera* mCamera;
        ID3D11Device* mDevice;
        ID3D11DeviceContext* mDeviceContext;

        HashID mModelHash;
        XMMATRIX mWorld;


        renderer::Mesh mMesh; // sphere model
        ID3D11SamplerState* mSampler;

        uint32 mLatLines;
        uint32 mLonLines;

    };
}
