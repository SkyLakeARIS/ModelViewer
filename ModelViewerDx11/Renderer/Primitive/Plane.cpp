#include "Plane.h"
#include "../Renderer.h"
#include "../../Util/Macro.h"

namespace renderer
{
    Plane::Plane()
        : mPosition(XMFLOAT3(0.0f, 0.0f, 0.0f))
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


        const DWORD indices[] =
        {
            0,
            1,
            2,
            0,
            2,
            3,
        };

        D3D11_BUFFER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0U;


        // vertex buffer
        D3D11_SUBRESOURCE_DATA subData;
        HRESULT result;

        desc.ByteWidth = sizeof(vertices);
        subData.pSysMem = vertices;

        ID3D11Device* device = Renderer::GetInstance()->GetDevice();

        result = device->CreateBuffer(&desc, &subData, &mVertexBuffers);
        if (FAILED(result))
        {
            ASSERT(false, "버텍스 버퍼 생성 실패 failed to create VertexBuffers");
        }


        // index buffer
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.ByteWidth = sizeof(indices);
        subData.pSysMem = indices;
        result = device->CreateBuffer(&desc, &subData, &mIndexBuffers);
        if (FAILED(result))
        {
            ASSERT(false, "인덱스 버퍼 생성 실패 failed to create IndexBuffers");
        }


        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.ByteWidth = sizeof(Renderer::CbWorld);
        result = Renderer::GetInstance()->CreateConstantBuffer(desc, &mCbWorld);
        if (FAILED(result))
        {
            ASSERT(false, "상수 버퍼 생성 실패 failed to create Constant Buffers");
        }


        // sampler
        D3D11_SAMPLER_DESC samplerDesc = {};
        // D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR  D3D11_FILTER_MIN_MAG_MIP_POINT
        samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 아직 정확히는 잘 모름.
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        result = device->CreateSamplerState(&samplerDesc, &mSamplerState);
        if (FAILED(result))
        {
            ASSERT(false, "SamplerState 생성 실패");
        }


        device->Release();
    }

    Plane::~Plane()
    {
        SAFETY_RELEASE(mTexture);
        SAFETY_RELEASE(mVertexBuffers);
        SAFETY_RELEASE(mIndexBuffers);
        SAFETY_RELEASE(mCbWorld);
        SAFETY_RELEASE(mSamplerState);
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

        constexpr uint32 stride = sizeof(VertexTex);
        constexpr uint32 offset = 0U;
        deviceContext->IASetVertexBuffers(0U, 1U, &mVertexBuffers, &stride, &offset);
        deviceContext->IASetIndexBuffer(mIndexBuffers, DXGI_FORMAT_R32_UINT, 0U);

        ID3D11ShaderResourceView* texDefault = mTexture;

        if (!mTexture)
        {
            texDefault = Renderer::GetInstance()->GetDefaultTexture();
            texDefault->Release();
        }
        deviceContext->PSSetSamplers(0U, 1U, &mSamplerState);
        deviceContext->PSSetShaderResources(0U, 1U, &texDefault);

        deviceContext->DrawIndexed(6, 0U, 0U);

        deviceContext->Release();
    }

    void Plane::DrawTexture(scene::Light* const light)
    {
        ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();

        Renderer::GetInstance()->SetInputLayoutTo(Renderer::eInputLayout::PT);
        Renderer::GetInstance()->SetShaderTo(Renderer::eShader::RenderToTexture);

        constexpr uint32 stride = sizeof(VertexTex);
        constexpr uint32 offset = 0U;
        deviceContext->IASetVertexBuffers(0U, 1U, &mVertexBuffers, &stride, &offset);
        deviceContext->IASetIndexBuffer(mIndexBuffers, DXGI_FORMAT_R32_UINT, 0U);

        XMMATRIX matWorld = XMMatrixTranspose(mMatWorld);
        Renderer::GetInstance()->UpdateCbTo(mCbWorld, &matWorld);
        Renderer::GetInstance()->BindCbToVsByObj(0, 1, &mCbWorld);
        Renderer::GetInstance()->BindCbToVsByType(1, 1, Renderer::eCbType::CbViewProj);

        deviceContext->PSSetSamplers(0U, 1U, &mSamplerState);
        SetTexture(Renderer::GetInstance()->GetShadowTexture());

        deviceContext->PSSetShaderResources(0U, 1U, &mTexture);

        deviceContext->DrawIndexed(6, 0U, 0U);

        ID3D11ShaderResourceView* const unbindSrv = nullptr;
        deviceContext->PSSetShaderResources(0U, 1U, &unbindSrv);
        UnbindTexture();
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

    void Plane::UnbindTexture()
    {
        SAFETY_RELEASE(mTexture);
        mTexture = nullptr;
    }
}