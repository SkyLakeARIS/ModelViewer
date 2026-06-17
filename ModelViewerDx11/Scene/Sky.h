#pragma once
#include "../framework.h"
#include "../Renderer/Resources/ModelData.h"

namespace renderer
{
    class TextureManager;
    class Renderer;
}

namespace scene
{
    class Camera;

    class Sky
    {
    public:

        Sky(Camera& camera);
        ~Sky();

        HRESULT Initialize(uint32 latLines, uint32 lonLines, renderer::TextureManager* const texManager);

        void Draw();
        void Update();
    private:

        HRESULT createSphere(uint32 latLines, uint32 lonLines);

    private:
        Camera* mCamera;

        HashID mModelHash;
        HashID mTextureHash;
        XMMATRIX mWorld;

        uint32 mLatLines;
        uint32 mLonLines;
    };
}
