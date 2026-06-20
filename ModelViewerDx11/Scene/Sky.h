#pragma once
#include "../framework.h"
#include "../Renderer/Resources/ModelData.h"

namespace renderer
{
    class Renderer;
    class TextureManager;
}

namespace scene
{
    class Camera;

    class Sky
    {
    public:

        Sky(Camera& camera);
        ~Sky();

        HRESULT Initialize(uint32 latLines, uint32 lonLines, renderer::TextureManager* const texManager, renderer::Renderer& renderer);

        void Draw(renderer::Renderer& renderer);
        void Update(renderer::Renderer& renderer);
    private:

        HRESULT createSphere(uint32 latLines, uint32 lonLines, renderer::Renderer& renderer);

    private:
        Camera* mCamera;

        renderer::Mesh mMesh;
        XMMATRIX mWorld;

        uint32 mLatLines;
        uint32 mLonLines;
    };
}
