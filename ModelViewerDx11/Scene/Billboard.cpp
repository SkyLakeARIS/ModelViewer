#include "Billboard.h"
#include "Camera.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Primitive/MeshGenerator.h"

namespace scene
{
    Billboard::Billboard()
        : mBlendHash(0)
        , mPosition()
        , mMatWorld(XMMatrixIdentity())
    {
        renderer::MeshGenerator::CreatePlane(mMesh);
    }

    Billboard::~Billboard()
    {
        // TODO: 추가한 MeshData 처리할 수 있는 로직이 필요함. - (종료될 떄 처리되기 때문에 당장 문제는 없음)
    }

    void Billboard::Initialize(renderer::Renderer& renderer)
    {
        // TODO: improve - 엄밀히 Material 정보에 포함되어야 할 것 같다. 
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = true;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        renderer.CreateBlendState(blendDesc, mBlendHash);
    }

    void Billboard::Draw(renderer::Renderer& renderer)
    {
        renderer::CbWorld cbWorld = { };
        const XMMATRIX matTranslate  = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
        const XMMATRIX matWorld = mMatWorld * matTranslate;
        cbWorld.Matrix = XMMatrixTranspose(matWorld);
        renderer.UpdateCB(renderer::eCbType::CbWorld, &cbWorld);


        renderer.BindInputLayoutTo(renderer::eInputLayout::PT);
        renderer.BindShaderTo(renderer::eShader::RenderToTexture);

        renderer.BindRasterStateByType(renderer::eRasterType::Basic);
        renderer.BindBlendStateByHash(mBlendHash, nullptr, 0xffffffff);

        renderer.BindCbToVsByType(0U, 1U, renderer::eCbType::CbWorld);
        renderer.BindCbToVsByType(1U, 1U, renderer::eCbType::CbViewProj);


        const int16_t strideVertex = GetVertexStrideSize(mMesh.VertexLayoutType);
        renderer.BindVertexBuffer(strideVertex);
        renderer.BindIndexBuffer();

        renderer.BindTextureToPs(0, mMesh.TextureHashes[static_cast<int8_t>(renderer::eTextureType::Diffuse)]);
        
        renderer.BindSamplerToPsByType(0, renderer::eSamplerType::AnisotropicWrap);

        renderer.DrawIndexed(mMesh.IndexRange.Count, mMesh.IndexRange.StartIndex, mMesh.VertexRange.StartIndex);
    }

    void Billboard::UpdateScaleMatrix(Camera& camera)
    {
        const XMMATRIX& matView = camera.GetViewMatrix();
        // row major
        (void)memcpy(&mMatWorld.r[0].m128_f32, &matView.r[0].m128_f32, sizeof(XMFLOAT3));
        (void)memcpy(&mMatWorld.r[1].m128_f32, &matView.r[1].m128_f32, sizeof(XMFLOAT3));
        (void)memcpy(&mMatWorld.r[2].m128_f32, &matView.r[2].m128_f32, sizeof(XMFLOAT3));

        XMVECTOR determinant;
        mMatWorld = XMMatrixInverse(&determinant, mMatWorld);
    }

    void Billboard::SetTexture(HashID texHash)
    {
        mMesh.TextureHashes[static_cast<int8_t>(renderer::eTextureType::Diffuse)] = texHash;
    }

    void Billboard::SetPosition(const XMFLOAT3& position)
    {
        mPosition = position;
    }
}
