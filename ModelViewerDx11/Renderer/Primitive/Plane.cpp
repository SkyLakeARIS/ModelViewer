#include "Plane.h"
#include "../Renderer.h"
#include "../../Util/Define.h"
#include "../../Util/Macro.h"
#include "../../Util/Util.h"
#include "../Resources/BufferManager.h"
#include "../Resources/ModelData.h"

namespace renderer
{
    std::atomic_int32_t Plane::sObjectCount(0);

    Plane::Plane(BufferManager* const bufferManager, renderer::Renderer& renderer)
        : mBufferManager(bufferManager)
        , mTexHash(0)
        , mPosition(XMFLOAT3(0.0f, 0.0f, 0.0f))
        , mScale(XMFLOAT3(1.6f, 1.6f, 0.6f))
        , mRotation(XMFLOAT3(0.0f, 0.0f, 0.0f))
        , mMatWorld(XMMatrixIdentity())
    {
        VertexTex vertices[] =
        {
            {XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT2(0.0f, 1.0f)},
            {XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT2(0.0f, 0.0f)},
            {XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT2(1.0f, 0.0f)},
            {XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT2(1.0f, 1.0f)},
        };

        DWORD indices[] =
        {
            0,
            1,
            2,
            0,
            2,
            3,
        };

        sObjectCount.fetch_add(1);

        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Plane_%d.mesh",
                  reinterpret_cast<const char*>(VIRTUAL_ROOT_PATH), sObjectCount.load());

        mModelHash = util::GetDjb2Hash(virtualFilePath);

        bufferManager->AddVertexData(reinterpret_cast<int8_t*>(vertices), sizeof(vertices), mModelHash);
        bufferManager->AddIndexData(reinterpret_cast<int8_t*>(indices), sizeof(indices), mModelHash);
    }

    Plane::~Plane()
    {
        mBufferManager->RemoveVertexData(mModelHash);
        mBufferManager->RemoveIndexData(mModelHash);
        mBufferManager = nullptr;
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
        BufferManager* const bufferManager = renderer.GetBufferManager();
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        const uint32 stride = sizeof(VertexTex);
        const uint32 offset = vertexRange.StartIndex;

        renderer.BindVertexBuffer(stride, offset);
        renderer.BindIndexBuffer(indexRange.StartIndex);

        if (!mTexHash)
        {
            renderer.BindDefaultTextureToPs(0);
        }
        else
        {
            renderer.BindTextureToPs(0, mTexHash);
        }
        renderer.BindSamplerToPsByType(0, eSamplerType::AnisotropicWrap);

        renderer.DrawIndexed(6, 0U, 0U);

    }

    void Plane::DrawTexture(renderer::Renderer& renderer)
    {

        renderer.BindInputLayoutTo(eInputLayout::PT);
        renderer.BindShaderTo(eShader::RenderToTexture);

        BufferManager* const bufferManager = renderer.GetBufferManager();
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        const uint32 stride = sizeof(VertexTex);
        const uint32 offset = vertexRange.StartIndex;

        renderer.BindVertexBuffer(stride, offset);
        renderer.BindIndexBuffer(indexRange.StartIndex);

        XMMATRIX matWorld = XMMatrixTranspose(mMatWorld);
        renderer.UpdateCB(eCbType::CbWorld, &matWorld);
        renderer.BindCbToVsByType(0, 1, eCbType::CbWorld);
        renderer.BindCbToVsByType(1, 1, eCbType::CbViewProj);

        renderer.BindSamplerToPsByType(0, eSamplerType::AnisotropicWrap);

        renderer.BindShadowTextureToPs(0);
        renderer.DrawIndexed(6, 0U, 0U);

        renderer.UnbindTexturePs(0);
    }

    void Plane::Update()
    {
        mMatWorld = XMMatrixIdentity();
        mMatWorld = XMMatrixScaling(mScale.x, mScale.y, mScale.z);
        mMatWorld = mMatWorld * XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
    }

    void Plane::SetPosition(XMFLOAT3& pos)
    {
        mPosition = pos;
    }

    void Plane::SetScale(XMFLOAT3& scale)
    {
        mScale = scale;
    }

    void Plane::SetTexHash(HashID textureHash)
    {
        mTexHash = textureHash;
    }
}
