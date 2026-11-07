#include "Billboard.h"
#include "../Renderer/Renderer.h"
#include "../Util/Macro.h"


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

    void Billboard::SubmitCommand(renderer::Renderer& renderer)
    {
        const XMMATRIX matTranslate = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
        const XMMATRIX matWorld = XMMatrixTranspose(mMatWorld * matTranslate);

        renderer::PushConstantBlock pushConstantBlock{ matWorld };
        renderer.UploadPushConstant(pushConstantBlock, renderer::eShader::Texture);
        const auto& descriptorSetIt = mDescriptorSets.find(renderer::eShader::Texture);
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

    void Billboard::UpdateScaleMatrix(const XMMATRIX& viewMatrix)
    {
        // row major
        (void)memcpy(&mMatWorld.r[0].m128_f32, &viewMatrix.r[0].m128_f32, sizeof(XMFLOAT3));
        (void)memcpy(&mMatWorld.r[1].m128_f32, &viewMatrix.r[1].m128_f32, sizeof(XMFLOAT3));
        (void)memcpy(&mMatWorld.r[2].m128_f32, &viewMatrix.r[2].m128_f32, sizeof(XMFLOAT3));

        XMVECTOR determinant;
        mMatWorld = XMMatrixInverse(&determinant, mMatWorld);
    }

    void Billboard::SetPosition(const XMFLOAT3& position)
    {
        mPosition = position;
    }

    void Billboard::SetMesh(const renderer::Mesh& mesh)
    {
        mMesh = std::move(mesh);
    }

    void Billboard::SetDescriptorSets(renderer::eShader shaderType, std::vector<renderer::DescriptorSet*>& descriptorSets)
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
