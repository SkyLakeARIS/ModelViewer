#include "Sky.h"
#include "../Renderer/Primitive/MeshGenerator.h"
#include "../Renderer/Renderer.h"
#include "../Util/Macro.h"

namespace scene
{
    Sky::Sky()
        : mMesh()
        , mWorld(XMMatrixIdentity())
    {
    }

    Sky::~Sky()
    {}

    bool Sky::Initialize(uint32_t latLines, uint32_t lonLines, renderer::TextureManager* const texManager)
    {

        return true;
    }

    void Sky::SubmitCommand(renderer::Renderer& renderer)
    {
        renderer::PushConstantBlock pushConstantBlock{ mWorld };
        renderer.UploadPushConstant(pushConstantBlock, renderer::eShader::Skybox);
        const auto& descriptorSetIt = mDescriptorSets.find(renderer::eShader::Skybox);
        auto descriptorSet = descriptorSetIt->second.begin();
        for (const auto& subMesh : mMesh.SubMeshes)
        {
            const auto& pipelineIt = subMesh.Pipelines.find(renderer::eRenderPass::Main);
            renderer.BindPipeline(pipelineIt->second);
            renderer.BindDescriptorSet(*descriptorSet);
            renderer.DrawIndexed(subMesh.IndexRange.Count, subMesh.VertexRange.StartIndex, subMesh.IndexRange.StartIndex);
            ++descriptorSet;
        }
    }

    void Sky::Update(const XMFLOAT3& cameraPosition)
    {
        // update
        constexpr float SKY_SCALE_SIZE = 100.0f;
        const XMMATRIX matScale = XMMatrixScaling(SKY_SCALE_SIZE, SKY_SCALE_SIZE, SKY_SCALE_SIZE);
        const XMMATRIX matTranslate = XMMatrixTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z);

        mWorld = XMMatrixTranspose(matScale * matTranslate);

    }

    void Sky::SetMesh(const renderer::Mesh& mesh)
    {
        mMesh = std::move(mesh);
    }

    void Sky::SetDescriptorSets(renderer::eShader shaderType, std::vector<renderer::DescriptorSet*>& descriptorSets)
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
