#include "Plane.h"
#include "MeshGenerator.h"
#include "../Renderer.h"

namespace renderer
{

    Plane::Plane()
        : mMesh()
        , mPosition(XMFLOAT3(0.0f, 0.0f, 0.0f))
        , mScale(XMFLOAT3(1.6f, 1.6f, 0.6f))
        , mRotation(XMFLOAT3(0.0f, 0.0f, 0.0f))
        , mMatWorld(XMMatrixIdentity())
    {
        MeshGenerator::CreatePlane(mMesh);
    }

    Plane::~Plane()
    {
        // TODO: 추가한 BufferData 처리할 수 있는 로직이 필요함. - (종료될 떄 처리되기 때문에 당장 문제는 없음)
    }

    XMFLOAT3 Plane::GetPosition() const
    {
        return mPosition;
    }

    XMFLOAT3 Plane::GetScale() const
    {
        return mScale;
    }

    void Plane::GetWorldMatrix(XMMATRIX& outMat) const
    {
        outMat = mMatWorld;
    }

    void Plane::Draw(renderer::Renderer& renderer)
    {
        const int16_t strideVertex = GetVertexStrideSize(mMesh.VertexLayoutType);

        renderer.BindVertexBuffer(strideVertex);
        renderer.BindIndexBuffer();

        if (!mMesh.TextureHashes[static_cast<int8_t>(eTextureType::Diffuse)])
        {
            renderer.BindDefaultTextureToPs(0);
        }
        else
        {
            renderer.BindTextureToPs(0, mMesh.TextureHashes[static_cast<int8_t>(eTextureType::Diffuse)]);
        }
        renderer.BindSamplerToPsByType(0, eSamplerType::AnisotropicWrap);

        renderer.DrawIndexed(mMesh.IndexRange.Count, mMesh.IndexRange.StartIndex, mMesh.VertexRange.StartIndex);
    }

    void Plane::DrawTexture(renderer::Renderer& renderer)
    {
        renderer.BindInputLayoutTo(eInputLayout::PT);
        renderer.BindShaderTo(eShader::RenderToTexture);

        const int16_t strideVertex = GetVertexStrideSize(mMesh.VertexLayoutType);
        renderer.BindVertexBuffer(strideVertex);
        renderer.BindIndexBuffer();

        XMMATRIX matWorld = XMMatrixTranspose(mMatWorld);
        renderer.UpdateCB(eCbType::CbWorld, &matWorld);
        renderer.BindCbToVsByType(0, 1, eCbType::CbWorld);
        renderer.BindCbToVsByType(1, 1, eCbType::CbViewProj);

        renderer.BindSamplerToPsByType(0, eSamplerType::AnisotropicWrap);

        renderer.BindShadowTextureToPs(0);
        renderer.DrawIndexed(mMesh.IndexRange.Count, mMesh.IndexRange.StartIndex, mMesh.VertexRange.StartIndex);

        renderer.UnbindTexturePs(0);
    }

    void Plane::Update()
    {
        mMatWorld = XMMatrixIdentity();
        mMatWorld = XMMatrixScaling(mScale.x, mScale.y, mScale.z);
        mMatWorld = mMatWorld * XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
    }

    void Plane::SetPosition(const XMFLOAT3& pos)
    {
        mPosition = pos;
    }

    void Plane::SetScale(const XMFLOAT3& scale)
    {
        mScale = scale;
    }

    void Plane::SetTexHash(HashID textureHash)
    {
        mMesh.TextureHashes[static_cast<int8_t>(eTextureType::Diffuse)] = textureHash;
    }
}
