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

        HRESULT Initialize(uint32 latLines, uint32 lonLines, renderer::TextureManager* const texManager);

        void Draw(renderer::Renderer& renderer);
        void Update(renderer::Renderer& renderer);

    private:
        Camera* mCamera;

        renderer::Mesh mMesh;
        XMMATRIX mWorld;

    };
}
