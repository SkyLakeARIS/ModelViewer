#pragma once
#include "../Renderer/Resources/Mesh.h"


namespace renderer
{
}

namespace scene
{
    class Camera;

    class Billboard
    {
    public:
        Billboard();
        ~Billboard();

        void Initialize();

        void SubmitCommand();

        void UpdateScaleMatrix(const XMMATRIX& viewMatrix);

        void SetTexture(HashID texHash, int16_t texSerial);
        void SetPosition(const XMFLOAT3& position);
    private:
        renderer::Mesh mMesh;
        HashID mBlendHash;
        XMFLOAT3 mPosition;
        XMMATRIX mMatWorld;
    };

}
