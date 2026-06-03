#pragma once
#include "../../framework.h"

namespace scene
{
    class Light;
}

namespace renderer
{
    // TODO: UI, debug panel 용도 분리 필요, Plane mesh 자체도 Generator가 담당하도록 
    class Plane final
    {
    public:
        struct VertexTex // 4bytes align
        {
            XMFLOAT3 Position;
            XMFLOAT2 TexCoord;
        };
    public:

        Plane();
        ~Plane();

        void Draw();
        void DrawTexture(scene::Light* const light);
        void Update();

        void SetPosition(XMFLOAT3& pos);
        void SetScale(XMFLOAT3& scale);
        // tex ref count is increased internally
        void SetTexture(ID3D11ShaderResourceView* const tex);
        void UnbindTexture();

        XMFLOAT3 GetPosition() const;
        XMFLOAT3 GetScale() const;
        void GetWorldMatrix(XMMATRIX& outMat) const;

    private:
        static std::atomic_int32_t sObjectCount;
    private:
        ID3D11Buffer* mCbWorld;
        ID3D11SamplerState* mSamplerState;

        HashID mModelHash;
        VertexTex mMesh;
        XMFLOAT3 mPosition;
        XMFLOAT3 mScale;
        XMFLOAT3 mRotation;

        XMMATRIX mMatWorld;
        ID3D11ShaderResourceView* mTexture;
    };
}
