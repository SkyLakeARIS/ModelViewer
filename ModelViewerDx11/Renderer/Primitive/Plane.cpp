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

    Plane::Plane()
        : mTexHash(0)
        , mPosition(XMFLOAT3(0.0f, 0.0f, 0.0f))
        , mScale(XMFLOAT3(1.6f, 1.6f, 0.6f))
        , mRotation(XMFLOAT3(0.0f, 0.0f, 0.0f))
        , mMatWorld(XMMatrixIdentity())
        , mTexture(nullptr)
    {
        VertexTex vertices[] =
        {
            {XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT2(0.0f, 1.0f)},
            {XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT2(0.0f, 0.0f)},
            {XMFLOAT3(0.5f, 0.5f, 0.5f),  XMFLOAT2(1.0f, 0.0f)},
            { XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT2(1.0f, 1.0f)},
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
                  reinterpret_cast<const char*>(renderer::VIRTUAL_ROOT_PATH), sObjectCount.load());

        mModelHash = util::GetDjb2Hash(virtualFilePath);

        renderer::BufferManager* const bufferManager = renderer::Renderer::GetInstance()->GetBufferManager();
        bufferManager->AddVertexData(reinterpret_cast<int8_t*>(vertices), sizeof(vertices), mModelHash);
        bufferManager->AddIndexData(reinterpret_cast<int8_t*>(indices), sizeof(indices), mModelHash);


        // vertex buffer
        HRESULT result;
        ID3D11Device* device = Renderer::GetInstance()->GetDevice();





        device->Release();
    }

    Plane::~Plane()
    {
        renderer::BufferManager* const bufferManager = renderer::Renderer::GetInstance()->GetBufferManager();
        bufferManager->RemoveVertexData(mModelHash);
        bufferManager->RemoveIndexData(mModelHash);

        SAFETY_RELEASE(mTexture);
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

    void Plane::Draw()
    {
        ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();

        BufferManager* const bufferManager = Renderer::GetInstance()->GetBufferManager();
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        const uint32 stride = sizeof(VertexTex);
        const uint32 offset = vertexRange.StartIndex;

        Renderer::GetInstance()->BindVertexBuffer(stride, offset);
        Renderer::GetInstance()->BindIndexBuffer(indexRange.StartIndex);

        ID3D11ShaderResourceView* texDefault = mTexture;

        if (!mTexture)
        {
            texDefault = Renderer::GetInstance()->GetDefaultTexture();
            texDefault->Release();
        }
        Renderer::GetInstance()->BindSamplerToPsByType(0, Renderer::eSamplerType::AnisotropicWrap);
        deviceContext->PSSetShaderResources(0U, 1U, &texDefault);

        deviceContext->DrawIndexed(6, 0U, 0U);

        deviceContext->Release();
    }

    void Plane::DrawNew()
    {
        ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();

        BufferManager* const bufferManager = Renderer::GetInstance()->GetBufferManager();
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        const uint32 stride = sizeof(VertexTex);
        const uint32 offset = vertexRange.StartIndex;

        Renderer::GetInstance()->BindVertexBuffer(stride, offset);
        Renderer::GetInstance()->BindIndexBuffer(indexRange.StartIndex);

        if (!mTexHash)
        {
            Renderer::GetInstance()->BindDefaultTextureToPs(0);
        }
        else
        {
            Renderer::GetInstance()->BindTextureToPs(0, mTexHash);
        }
        Renderer::GetInstance()->BindSamplerToPsByType(0, Renderer::eSamplerType::AnisotropicWrap);

        deviceContext->DrawIndexed(6, 0U, 0U);

        deviceContext->Release();
    }

    void Plane::DrawTexture(scene::Light* const light)
    {
        ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();

        Renderer::GetInstance()->SetInputLayoutTo(Renderer::eInputLayout::PT);
        Renderer::GetInstance()->SetShaderTo(Renderer::eShader::RenderToTexture);

        BufferManager* const bufferManager = Renderer::GetInstance()->GetBufferManager();
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        const uint32 stride = sizeof(VertexTex);
        const uint32 offset = vertexRange.StartIndex;

        Renderer::GetInstance()->BindVertexBuffer(stride, offset);
        Renderer::GetInstance()->BindIndexBuffer(indexRange.StartIndex);

        XMMATRIX matWorld = XMMatrixTranspose(mMatWorld);
        Renderer::GetInstance()->UpdateCB(Renderer::eCbType::CbWorld, &matWorld);
        Renderer::GetInstance()->BindCbToVsByType(0, 1, Renderer::eCbType::CbWorld);
        Renderer::GetInstance()->BindCbToVsByType(1, 1, Renderer::eCbType::CbViewProj);

        Renderer::GetInstance()->BindSamplerToPsByType(0, Renderer::eSamplerType::AnisotropicWrap);
        SetTexture(Renderer::GetInstance()->GetShadowTexture());

        deviceContext->PSSetShaderResources(0U, 1U, &mTexture);

        deviceContext->DrawIndexed(6, 0U, 0U);

        ID3D11ShaderResourceView* const unbindSrv = nullptr;
        deviceContext->PSSetShaderResources(0U, 1U, &unbindSrv);
        UnbindTexture();
        deviceContext->Release();
    }

    void Plane::DrawTextureNew(scene::Light* const light)
    {
        ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();

        Renderer::GetInstance()->SetInputLayoutTo(Renderer::eInputLayout::PT);
        Renderer::GetInstance()->SetShaderTo(Renderer::eShader::RenderToTexture);

        BufferManager* const bufferManager = Renderer::GetInstance()->GetBufferManager();
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        const uint32 stride = sizeof(VertexTex);
        const uint32 offset = vertexRange.StartIndex;

        Renderer::GetInstance()->BindVertexBuffer(stride, offset);
        Renderer::GetInstance()->BindIndexBuffer(indexRange.StartIndex);

        XMMATRIX matWorld = XMMatrixTranspose(mMatWorld);
        Renderer::GetInstance()->UpdateCB(Renderer::eCbType::CbWorld, &matWorld);
        Renderer::GetInstance()->BindCbToVsByType(0, 1, Renderer::eCbType::CbWorld);
        Renderer::GetInstance()->BindCbToVsByType(1, 1, Renderer::eCbType::CbViewProj);

        Renderer::GetInstance()->BindSamplerToPsByType(0, Renderer::eSamplerType::AnisotropicWrap);

        Renderer::GetInstance()->BindShadowTextureToPs(0);
        deviceContext->DrawIndexed(6, 0U, 0U);

        Renderer::GetInstance()->UnbindTexturePs(0);
        deviceContext->Release();
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

    void Plane::SetTexture(ID3D11ShaderResourceView* const tex)
    {
        ASSERT(tex != nullptr, "Plane ) do not pass nullptr.");
        mTexture = tex;

    }

    void Plane::SetTexHash(HashID textureHash)
    {
        mTexHash = textureHash;
    }

    void Plane::UnbindTexture()
    {
        SAFETY_RELEASE(mTexture);
        mTexture = nullptr;
    }
}
