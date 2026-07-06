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

    class Billboard
    {
    public:
        // TODO: 나중에 각 billboard 개체들 구분을 위한 이름같은 식별자 추가 필요
        Billboard();
        ~Billboard();

        void Initialize(renderer::Renderer& renderer);

        void Draw(renderer::Renderer& renderer);

        void UpdateScaleMatrix(Camera& camera);

        void SetTexture(HashID texHash);
        void SetPosition(const XMFLOAT3& position);
    private:
        renderer::Mesh mMesh;
        HashID mBlendHash;
        XMFLOAT3 mPosition;
        XMMATRIX mMatWorld;
    };

}
