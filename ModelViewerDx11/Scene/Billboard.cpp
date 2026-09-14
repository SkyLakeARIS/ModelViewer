#include "Billboard.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Primitive/MeshGenerator.h"
#include "../Renderer/Resources/RenderPacket.h"

namespace renderer
{
    struct RenderPacket;
}

namespace scene
{
    Billboard::Billboard()
        : mMesh()
        , mBlendHash(0)
        , mPosition()
        , mMatWorld(XMMatrixIdentity())
    {}

    Billboard::~Billboard()
    {
    }

    void Billboard::Initialize()
    {

        renderer::MeshGenerator::CreatePlane(mMesh);
        mMesh.SubMeshes.front().Material.Factors.IsLitOn = static_cast<float>(false);
    }

    void Billboard::SubmitCommand(std::vector<renderer::RenderPacket>& commandList)
    {
        const XMMATRIX matTranslate = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
        const XMMATRIX matWorld = mMatWorld * matTranslate;

        for (const auto& subMesh : mMesh.SubMeshes)
        {
            renderer::RenderPacket command = renderer::RenderPacket::MakeCommand(
                mMesh.VertexFormat,
                renderer::GetVertexStrideSize(mMesh.VertexFormat),
                renderer::eBufferUsage::Static,
                true,
                subMesh.VertexRange,
                subMesh.IndexRange,
                subMesh.Material,
                renderer::eRenderPass::Main,
                XMMatrixTranspose(matWorld),
                renderer::eShader::Texture,
                renderer::eRasterType::Basic,
                renderer::eSamplerType::AnisotropicWrap,
                renderer::eBlendState::AlphaBlend,
                renderer::ePrimitiveTopology::TriangleStrip,
                false,
                renderer::eDepthStencilState::DepthOffStencilOff
            );
            commandList.push_back(command);
        }
    }

    void Billboard::UpdateScaleMatrix(const XMMATRIX& viewMatrix)
    {
        // row major
        (void)memcpy(&mMatWorld.r[0].m128_f32, &viewMatrix.r[0].m128_f32, sizeof(XMFLOAT3));
        (void)memcpy(&mMatWorld.r[1].m128_f32, &viewMatrix.r[1].m128_f32, sizeof(XMFLOAT3));
        (void)memcpy(&mMatWorld.r[2].m128_f32, &viewMatrix.r[2].m128_f32, sizeof(XMFLOAT3));

        XMVECTOR determinant;
        mMatWorld = XMMatrixInverse(&determinant, mMatWorld);
    }

    void Billboard::SetTexture(HashID texHash, int16_t texSerial)
    {
        mMesh.SubMeshes.front().Material.TextureHashes[static_cast<int8_t>(renderer::eTextureType::Diffuse)] = texHash;
        mMesh.SubMeshes.front().Material.TextureSerials[static_cast<int8_t>(renderer::eTextureType::Diffuse)] = texSerial;
    }

    void Billboard::SetPosition(const XMFLOAT3& position)
    {
        mPosition = position;
    }
}
