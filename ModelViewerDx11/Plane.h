#pragma once
#include "framework.h"
#include "Model.h"

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
    void DrawTexture(Light* const light);
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
    ID3D11Buffer* mVertexBuffers;
    ID3D11Buffer* mIndexBuffers;
    ID3D11Buffer* mCbWorld;
    ID3D11SamplerState* mSamplerState;

    VertexTex mMesh;
    XMFLOAT3 mPosition;
    XMFLOAT3 mScale;
    XMFLOAT3 mRotation;

    XMMATRIX mMatWorld;
    ID3D11ShaderResourceView* mTexture;
};
