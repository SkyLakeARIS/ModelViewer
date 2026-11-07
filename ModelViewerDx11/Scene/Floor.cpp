#include "Floor.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Shader/DescriptorSet.h"
#include "../Util/Macro.h"

namespace scene
{
    Floor::Floor()
    {
    }

    Floor::~Floor()
    {
    }

    void Floor::SubmitCommand(renderer::Renderer& renderer)
    {
        const XMMATRIX matWorld = XMMatrixIdentity();

        renderer::PushConstantBlock pushConstantBlock{ matWorld };
        renderer.UploadPushConstant(pushConstantBlock, renderer::eShader::Color);
        const auto& descriptorSetIt = mDescriptorSets.find(renderer::eShader::Color);
        auto descriptorSet = descriptorSetIt->second.begin();
        std::vector<renderer::UniformWriteInfo> uniformWriteInfos;
        uniformWriteInfos.reserve(mMesh.SubMeshes.size());
        for (auto& subMesh : mMesh.SubMeshes)
        {
            renderer::UniformWriteInfo uniformWriteInfo{ *descriptorSet, reinterpret_cast<void*>(&subMesh.Material.Factors), {0, static_cast<int32_t>(sizeof(renderer::MaterialFactors))} };
            uniformWriteInfos.push_back(uniformWriteInfo);
            ++descriptorSet;
        }
        renderer.UpdateUniformData(uniformWriteInfos);

        descriptorSet = descriptorSetIt->second.begin();
        for (const auto& subMesh : mMesh.SubMeshes)
        {
            const auto& pipelineIt = subMesh.Pipelines.find(renderer::eRenderPass::Main);
            renderer.BindPipeline(pipelineIt->second);
            renderer.BindDescriptorSet(*descriptorSet);
            renderer.DrawIndexed(subMesh.IndexRange.Count, subMesh.VertexRange.StartIndex, subMesh.IndexRange.StartIndex);
            ++descriptorSet;
        }
    }

    void Floor::SetMesh(const renderer::Mesh& mesh)
    {
        mMesh = std::move(mesh);
    }

    void Floor::SetDescriptorSets(renderer::eShader shaderType, std::vector<renderer::DescriptorSet*>& descriptorSets)
    {
        ASSERT(mDescriptorSets.empty(), "Descriptors를 여러 번 초기화 하는 것 같습니다.");
        auto descriptorSetIt = mDescriptorSets.find(shaderType);
        if (descriptorSetIt == mDescriptorSets.end())
        {
            mDescriptorSets.insert({ shaderType, std::move(descriptorSets) });
        }
        else
        {
            descriptorSetIt->second.insert(descriptorSetIt->second.end(), descriptorSets.begin(), descriptorSets.end());
        }
    }
}
