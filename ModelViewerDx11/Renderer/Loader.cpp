#include "Loader.h"
#include "Renderer.h"
#include "../Util/Macro.h"
#include "../Util/Util.h"
#include "Importer/ImportedModelData.h"
#include "Importer/ModelImporter.h"
#include "Resources/Mesh.h"
#include "Resources/Model.h"
#include "Resources/ResourceManager.h"
#include "Resources/Texture.h"
#include "Shader/DescriptorAllocator.h"
#include "Shader/DescriptorLayout.h"
#include "Shader/DescriptorSet.h"

namespace renderer
{
    Loader::Loader(ModelImporter* const importer, Renderer* const renderer, ResourceManager* const resourceManager, DescriptorAllocator* const descriptorAllocator)
        : mImporter(importer)
        , mRenderer(renderer)
        , mResourceManager(resourceManager)
        , mDescriptorAllocator(descriptorAllocator)
    {
    }

    Loader::~Loader()
    {
        mRenderer = nullptr;
        mResourceManager = nullptr;
        mDescriptorAllocator = nullptr;
    }

    bool Loader::Load(const int8_t* const filePath, Model** outModel) const
    {
        HashID modelHash = 0;
        renderer::ImportedModelContainer modelContainer;
        mImporter->LoadFbxModel(filePath, modelHash, modelContainer);

        renderer::Mesh newMesh = {};
        newMesh.SubMeshes.reserve(modelContainer.SubMeshes.size());
        newMesh.VertexFormat = renderer::eVertexFormat::PTN;
        const int16_t strideVertex = GetVertexStrideSize(newMesh.VertexFormat);
        const int16_t strideIndex = sizeof(uint32_t);
        const int16_t filePathLength = static_cast<int16_t>(strlen(reinterpret_cast<char const*>(filePath)));
        ASSERT(filePathLength + 1 <= util::MAX_NAME_LENGTH, "str이 버퍼 사이즈보다 큼. 입력 문자열을 검점하거나, util의 Length 상수 조정 필요. str(%d)", filePathLength + 1);
        (void)memcpy(newMesh.MeshName, filePath, filePathLength + 1);
        newMesh.MeshHash = modelHash;

        int32_t totalVertexCount = 0;
        int32_t totalIndexCount = 0;
        std::vector<DescriptorSet*> descriptorSets;
        for (auto& subMesh : modelContainer.SubMeshes)
        {
            renderer::SubMesh newSubMesh = {};

            newSubMesh.SubMeshHash = util::GetDjb2Hash(subMesh.MeshName, modelHash);
            mRenderer->AddVertexData(subMesh.VertexBuffer.get(), sizeof(renderer::VertexPTN), subMesh.VertexCount, newSubMesh.SubMeshHash, newSubMesh.VertexRange);
            mRenderer->AddIndexData(subMesh.IndexBuffer.get(), sizeof(uint32_t), subMesh.IndexCount, newSubMesh.SubMeshHash, newSubMesh.IndexRange);

            for (EnumInt textureType = 0; textureType < static_cast<EnumInt>(renderer::eTextureType::TextureTypeCount); ++textureType)
            {
                if (subMesh.Textures[textureType].TextureHash > 0)
                {
                    newSubMesh.Material.TextureHashes[static_cast<EnumInt>(subMesh.Textures[textureType].TextureType)] = subMesh.Textures[textureType].TextureHash;
                    Texture* newTexture = new Texture(mRenderer->GetDevice());
                    newTexture->Create(subMesh.Textures[textureType].FilePath, false);
                    if(mResourceManager->TryAddTexture(subMesh.Textures[textureType].TextureHash, newTexture) == false)
                    {
                        delete newTexture;
                    }
                    newTexture = nullptr;
                }
            }

            totalVertexCount += newSubMesh.VertexRange.Count;
            totalIndexCount += newSubMesh.IndexRange.Count;
            (void)memcpy(newSubMesh.SubMeshName, subMesh.MeshName, util::MAX_NAME_LENGTH);

            newSubMesh.MinBound = std::move(subMesh.MinBound);
            newSubMesh.MaxBound = std::move(subMesh.MaxBound);

            newSubMesh.Material.Factors = std::move(subMesh.MaterialParam);
            newSubMesh.Material.Factors.IsLitOn = static_cast<float>(true);

            // MEMO: configure DescriptorSet
            const eLayoutType layoutType = DescriptorLayout::GetLayoutTypeByShaderType(eShader::Simple);
            DescriptorSet* descriptorSet = nullptr;
            if (mDescriptorAllocator->AllocateSet(layoutType, &descriptorSet) == false)
            {
                ASSERT(false, "Model 로드 중 DescriptorSet 구성 실패. filePath(%s)", filePath);
                return false;
            }
            std::vector<DescriptorWriteInfo> writeInfos;
            DescriptorLayout::GetLayoutWriteInfos(layoutType, eShader::Simple, writeInfos);
            for(auto& writeInfo : writeInfos)
            {
                // MEMO: 통합 버퍼 생성 전까지 자신의 버퍼를 연결한다.
                if(writeInfo.bTexture && writeInfo.TextureType != eTextureType::TextureTypeCount)
                {
                    writeInfo.Texture = mResourceManager->GetTexture(newSubMesh.Material.TextureHashes[static_cast<EnumInt>(writeInfo.TextureType)]);
                }
                else if(writeInfo.Descriptor.Type == VK_DESCRIPTOR_TYPE_SAMPLER)
                {
                    writeInfo.Sampler = mRenderer->GetSampler(eSamplerType::Linear);
                }
            }

            descriptorSet->UpdateDescriptorSet(writeInfos);
            descriptorSets.push_back(descriptorSet);

            newMesh.SubMeshes.push_back(std::move(newSubMesh));
        }

        newMesh.VertexRange.StartIndex = newMesh.SubMeshes.front().VertexRange.StartIndex;
        newMesh.VertexRange.Count = totalVertexCount;

        newMesh.IndexRange.StartIndex = newMesh.SubMeshes.front().IndexRange.StartIndex;
        newMesh.IndexRange.Count = totalIndexCount;

        *outModel = new Model();
        (*outModel)->SetMesh(newMesh);
        (*outModel)->SetDescriptorSets(eShader::Simple, descriptorSets);

        HashID simplePipelineHash;
        if(mRenderer->TryCreateGraphicsPipeline(simplePipelineHash, renderer::eShader::Simple, renderer::eVertexFormat::PTN, renderer::ePrimitiveTopology::Triangles, renderer::eRasterType::Basic, renderer::Opaque, renderer::eDepthStencilState::DepthOffStencilOff, renderer::eRenderPass::Main) == false)
        {
            delete *outModel;
            (*outModel) = nullptr;
            ASSERT(false, "Model 로드 중 파이프라인 구성 실패. filePath(%s)", filePath);
            return false;
        }

        return true;
    }
}
