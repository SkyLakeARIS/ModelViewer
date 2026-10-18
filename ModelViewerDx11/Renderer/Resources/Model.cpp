#include "Model.h"
#include "BufferManager.h"
#include "../Renderer.h"
#include "../../Util/Macro.h"
#include "../Resources/RenderPacket.h"

namespace renderer
{
    Model::Model(scene::Camera* camera, BufferManager* bufferManager)
        : mBufferManager(bufferManager)
        , mMesh()
        , mMatRotation(XMMatrixIdentity())
        , mMatScale(XMMatrixIdentity())
        , mbHighlight(false)
        , mbActiveEmissive(false)
    {
        ASSERT(camera != nullptr, "do not pass nullptr");

        mMatWorld  = XMMatrixIdentity();
    }

    Model::~Model()
    {
        const int16_t stride = GetVertexStrideSize(mMesh.VertexFormat);
        for(auto& subMesh : mMesh.SubMeshes)
        {
            const int16_t strideIndex = mBufferManager->GetIndexStrideSize();
            mBufferManager->RemoveVertexData(stride, subMesh.SubMeshHash);
            mBufferManager->RemoveIndexData(strideIndex, subMesh.SubMeshHash);
        }
        mBufferManager = nullptr;
    }

    void Model::SubmitCommand(std::vector<renderer::RenderPacket>& commandList)
    {
        // Shadow pass
        for (const auto& subMesh : mMesh.SubMeshes)
        {
            renderer::RenderPacket command = renderer::RenderPacket::MakeCommand(
                eVertexFormat::P,
                renderer::GetVertexStrideSize(mMesh.VertexFormat),
                renderer::eBufferUsage::Static,
                false,
                subMesh.VertexRange,
                subMesh.IndexRange,
                subMesh.Material,
                renderer::eRenderPass::Shadow,
                mMatWorld,
                renderer::eShader::Shadow,
                renderer::eRasterType::Outline,
                renderer::eSamplerType::SamplerCount,
                eBlendState::Opaque,
                renderer::ePrimitiveTopology::Triangles,
                false,
                eDepthStencilState::DepthOffStencilOff
            );

            commandList.push_back(command);
        }

        // outline
        if (mbHighlight)
        {

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
                    mMatWorld,
                    renderer::eShader::Outline,
                    renderer::eRasterType::CullBack,
                    renderer::eSamplerType::SamplerCount,
                    eBlendState::Opaque,
                    renderer::ePrimitiveTopology::Triangles,
                    false,
                    eDepthStencilState::DepthOnMaskAllCompLessEqual
                );

                if (!mbActiveEmissive)
                {
                    command.Material.Factors.Emissive = XMFLOAT3(0.0f, 0.0f, 0.0f);
                }

                commandList.push_back(command);
            }
        }

        // Main pass
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
                mMatWorld,
                renderer::eShader::BasicWithShadow,
                renderer::eRasterType::Basic,
                renderer::eSamplerType::AnisotropicWrap,
                eBlendState::Opaque,
                renderer::ePrimitiveTopology::Triangles,
                false,
                eDepthStencilState::DepthOffStencilOff
            );

            if (!mbActiveEmissive)
            {
                command.Material.Factors.Emissive = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }

            commandList.push_back(command);
        }
    }

    void Model::DrawVk(VkRenderer& renderer)
    {
        for (const auto& subMesh : mMesh.SubMeshes)
        {
            renderer.DrawIndexed(subMesh.IndexRange.Count, subMesh.VertexRange.StartIndex, subMesh.IndexRange.StartIndex);
        }
    }

    void Model::Update()
    {
        // MEMO: 전치 안 해도 되지만, 나중에 필요해질 테니 까먹지 않도록 미리 구성해둠.
        mMatWorld = XMMatrixTranspose(XMMatrixIdentity());
    }

    void Model::SetMesh(const Mesh& mesh)
    {
        mMesh = std::move(mesh);
    }

    void Model::SetHighlight(bool bSelection)
    {
        mbHighlight = bSelection;
    }

    int32_t Model::GetSubMeshCount() const
    {
        return mMesh.SubMeshes.size();
    }

    XMFLOAT3 Model::GetCenterPoint(int32_t subMeshIndex) const
    {
        ASSERT((subMeshIndex >= 0 && subMeshIndex < mMesh.SubMeshes.size()), "유효하지 않은 SubMeshIndex. pass(%d), validSubMeshCount(%d)", subMeshIndex, mMesh.SubMeshes.size());

        const XMVECTOR minBound = XMLoadFloat3(&mMesh.SubMeshes[subMeshIndex].MinBound);
        const XMVECTOR maxBound = XMLoadFloat3(&mMesh.SubMeshes[subMeshIndex].MaxBound);
        const XMVECTOR mid = (minBound + maxBound) * 0.5f;
        XMFLOAT3 centerPosition;
        XMStoreFloat3(&centerPosition, mid);
        return centerPosition;
    }

    XMMATRIX Model::GetMatrix() const
    {
        return mMatWorld;
    }
}
