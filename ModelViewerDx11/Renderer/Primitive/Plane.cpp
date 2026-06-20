#include "Plane.h"
#include "../Renderer.h"
#include "../../Util/Define.h"
#include "../../Util/Util.h"
#include "../Resources/BufferManager.h"

namespace renderer
{
    std::atomic_int32_t Plane::sObjectCount(0);

    Plane::Plane(BufferManager* const bufferManager)
        : mBufferManager(bufferManager)
        , mMesh()
        , mPosition(XMFLOAT3(0.0f, 0.0f, 0.0f))
        , mScale(XMFLOAT3(1.6f, 1.6f, 0.6f))
        , mRotation(XMFLOAT3(0.0f, 0.0f, 0.0f))
        , mMatWorld(XMMatrixIdentity())
    {
        VertexPT vertices[] =
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
        const int16_t wroteCount = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Plane_%d.mesh",
                  reinterpret_cast<const char*>(VIRTUAL_ROOT_PATH), sObjectCount.load());

        mMesh.MeshHash = util::GetDjb2Hash(virtualFilePath);
        (void)memcpy(mMesh.MeshName, virtualFilePath, wroteCount + 1);
        mMesh.VertexLayoutType = eInputLayout::PT;
        const int16_t strideVertex = GetVertexStrideSize(mMesh.VertexLayoutType);
        const int16_t strideIndex = bufferManager->GetIndexStrideSize();

        bufferManager->AddVertexData(reinterpret_cast<int8_t*>(vertices), sizeof(vertices), mMesh.MeshHash, strideVertex, mMesh.VertexRange);
        bufferManager->AddIndexData(reinterpret_cast<int8_t*>(indices), sizeof(indices), mMesh.MeshHash, strideIndex, mMesh.IndexRange);
    }

    Plane::~Plane()
    {
        const int16_t strideVertex = GetVertexStrideSize(mMesh.VertexLayoutType);
        const int16_t strideIndex = mBufferManager->GetIndexStrideSize();
        mBufferManager->RemoveVertexData(strideVertex, mMesh.MeshHash);
        mBufferManager->RemoveIndexData(strideIndex, mMesh.MeshHash);
        mBufferManager = nullptr;
        // TODO: BufferManager를 받지 않고, BufferData 처리할 수 있는 로직이 필요함.
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

        renderer.BindVertexBufferNew(strideVertex, 0);
        renderer.BindIndexBufferNew(0);



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
        renderer.BindVertexBufferNew(strideVertex, 0);
        renderer.BindIndexBufferNew(0);

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
        mMesh.TextureHashes[static_cast<int8_t>(eTextureType::Diffuse)] = textureHash;
    }
}
