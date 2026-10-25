#pragma once
#include "../Renderer/Resources/Mesh.h"

namespace renderer
{
    class TextureManager;
}

namespace scene
{

    class Sky
    {
    public:

        Sky();
        ~Sky();

        bool Initialize(uint32_t latLines, uint32_t lonLines, renderer::TextureManager* const texManager);

        void SubmitCommand();
        void Update(const XMFLOAT3& cameraPosition);

    private:
        renderer::Mesh mMesh;
        XMMATRIX mWorld;
    };
}
