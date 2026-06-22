#pragma once
#include "../../framework.h"
#include "../Resources/ModelData.h"

namespace renderer
{
    class Renderer;

    // TODO: UI, debug panel 용도 분리 필요
    class Plane final
    {
    public:

        Plane();
        ~Plane();

        void Draw(renderer::Renderer& renderer);
        void DrawTexture(renderer::Renderer& renderer);
        void Update();

        void SetPosition(XMFLOAT3& pos);
        void SetScale(XMFLOAT3& scale);
        // TODO: improve - Plane 역할을 분리하기 전까지 기존 구조 방식에 Hash를 주입받아 쓰는 것으로 바꿈
        void SetTexHash(HashID textureHash);

        XMFLOAT3 GetPosition() const;
        XMFLOAT3 GetScale() const;
        void GetWorldMatrix(XMMATRIX& outMat) const;

    private:
        Mesh mMesh;
        XMFLOAT3 mPosition;
        XMFLOAT3 mScale;
        XMFLOAT3 mRotation;

        XMMATRIX mMatWorld;
    };
}
