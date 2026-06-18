#pragma once
#include "../../framework.h"

namespace scene
{
    class Light;
}

namespace renderer
{
    class TextureManager;

    // TODO: UI, debug panel 용도 분리 필요, Plane mesh 자체도 Generator가 담당하도록 
    class Plane final
    {
    public:
        // TODO: cleanup, improve - RenderType으로 이동하고, 버텍스 속성 유형별로 이름을 변경하기
        struct VertexTex // 4bytes align
        {
            XMFLOAT3 Position;
            XMFLOAT2 TexCoord;
        };
    public:

        Plane();
        ~Plane();

        void Draw();
        void DrawTexture();
        void Update();

        void SetPosition(XMFLOAT3& pos);
        void SetScale(XMFLOAT3& scale);
        // TODO: improve - Plane 역할을 분리하기 전까지 기존 구조 방식에 Hash를 주입받아 쓰는 것으로 바꿈
        void SetTexHash(HashID textureHash);

        XMFLOAT3 GetPosition() const;
        XMFLOAT3 GetScale() const;
        void GetWorldMatrix(XMMATRIX& outMat) const;

    private:
        static std::atomic_int32_t sObjectCount;
    private:

        HashID mTexHash;
        HashID mModelHash;
        VertexTex mMesh;
        XMFLOAT3 mPosition;
        XMFLOAT3 mScale;
        XMFLOAT3 mRotation;

        XMMATRIX mMatWorld;
    };
}
