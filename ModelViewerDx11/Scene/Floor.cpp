#include "Floor.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Primitive/MeshGenerator.h"
#include "../Renderer/Resources/RenderPacket.h"
#include "../Util/Macro.h"

namespace scene
{
    Floor::Floor(XMFLOAT2 startPoint, uint32_t gapEachLine, uint32_t numLineX, uint32_t numLineY)
    {
        ASSERT(numLineX >= 2, "numLineX must be 2 or greater");
        ASSERT(numLineY >= 2, "numLineY must be 2 or greater");
        ASSERT(gapEachLine >= 1, "gapEachLine must be 1 or greater");

        renderer::MeshGenerator::CreateGrid(startPoint, numLineX, numLineY, gapEachLine, mMesh);
        for (auto& subMesh : mMesh.SubMeshes)
        {
            subMesh.Material.Factors.Diffuse = XMFLOAT3(0.80f, 0.72f, 0.56f);
            subMesh.Material.Factors.IsLitOn = static_cast<float>(true);
            subMesh.Material.Factors.Ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
            subMesh.Material.Factors.Shininess = 8;
            subMesh.Material.Factors.Specular = XMFLOAT3(0.16f, 0.144f, 0.112f);
        }
    }

    Floor::~Floor()
    {
    }

    void Floor::SubmitCommand(std::vector<renderer::RenderPacket>& commandList)
    {
        XMMATRIX mat = XMMatrixIdentity();

        for (const auto& subMesh : mMesh.SubMeshes)
        {
            renderer::RenderPacket command = renderer::RenderPacket::MakeCommand(
                renderer::eVertexFormat::P,
                renderer::GetVertexStrideSize(mMesh.VertexFormat),
                renderer::eBufferUsage::Static,
                false,
                subMesh.VertexRange,
                subMesh.IndexRange,
                subMesh.Material,
                renderer::eRenderPass::Shadow,
                mat,
                renderer::eShader::Shadow,
                renderer::eRasterType::Basic,
                renderer::eSamplerType::SamplerCount,
                renderer::eBlendState::Opaque,
                renderer::ePrimitiveTopology::Triangles,
                false,
                renderer::eDepthStencilState::DepthOffStencilOff
            );

            commandList.push_back(command);
        }

        for (const auto& subMesh : mMesh.SubMeshes)
        {
            renderer::RenderPacket command = renderer::RenderPacket::MakeCommand(
                mMesh.VertexFormat,
                renderer::GetVertexStrideSize(mMesh.VertexFormat),
                renderer::eBufferUsage::Static,
                false,
                subMesh.VertexRange,
                subMesh.IndexRange,
                subMesh.Material,
                renderer::eRenderPass::Main,
                mat,
                renderer::eShader::Color,
                renderer::eRasterType::Basic,
                renderer::eSamplerType::SamplerCount,
                renderer::eBlendState::Opaque,
                renderer::ePrimitiveTopology::Triangles,
                false,
                renderer::eDepthStencilState::DepthOffStencilOff
            );

            commandList.push_back(command);
        }
    }
}
