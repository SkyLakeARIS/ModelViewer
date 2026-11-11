#include "Loader.h"
#include "Renderer.h"
#include "../Scene/Billboard.h"
#include "../Scene/Floor.h"
#include "../Scene/Sky.h"
#include "../Util/Macro.h"
#include "../Util/Util.h"
#include "Importer/ImportedModelData.h"
#include "Importer/ModelImporter.h"
#include "Primitive/MeshGenerator.h"
#include "Resources/Mesh.h"
#include "Resources/Model.h"
#include "Resources/ResourceManager.h"
#include "Resources/Texture.h"
#include "Shader/DescriptorAllocator.h"
#include "Shader/DescriptorLayout.h"
#include "Shader/DescriptorSet.h"

namespace renderer
{
    Loader::Loader(ModelImporter* const importer, Renderer* const renderer, DescriptorAllocator* const descriptorAllocator)
        : mImporter(importer)
        , mRenderer(renderer)
        , mResourceManager(renderer->GetResourceManager())
        , mDescriptorAllocator(descriptorAllocator)
    {
        ASSERT(mResourceManager, "ResourceManager가 초기화 되지 않은 것 같습니다.");
    }

    Loader::~Loader()
    {
        mImporter = nullptr;
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
        std::vector<DescriptorSet*> shadowDescriptorSets;
        for (auto& subMesh : modelContainer.SubMeshes)
        {
            renderer::SubMesh newSubMesh = {};

            newSubMesh.SubMeshHash = util::GetDjb2Hash(subMesh.MeshName, modelHash);
            mResourceManager->AddBufferData(subMesh.VertexBuffer.get(), sizeof(renderer::VertexPTN), subMesh.VertexCount, newSubMesh.SubMeshHash, newSubMesh.VertexRange, true);
            mResourceManager->AddBufferData(subMesh.IndexBuffer.get(), sizeof(uint32_t), subMesh.IndexCount, newSubMesh.SubMeshHash, newSubMesh.IndexRange, false);

            for (EnumInt textureType = 0; textureType < static_cast<EnumInt>(renderer::eTextureType::TextureTypeCount); ++textureType)
            {
                if (subMesh.Textures[textureType].TextureHash > 0)
                {
                    if(mResourceManager->TryGetTexture(subMesh.Textures[textureType].TextureHash) == nullptr)
                    {
                        Texture* newTexture = new Texture(mRenderer->GetDevice());
                        if(newTexture->Create(subMesh.Textures[textureType].FilePath, false) == false)
                        {
                            delete newTexture;
                            return false;
                        }
                        mResourceManager->AddTexture(subMesh.Textures[textureType].TextureHash, newTexture);
                    }
                    newSubMesh.Material.TextureHashes[textureType] = subMesh.Textures[textureType].TextureHash;
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
            eLayoutType layoutType = DescriptorLayout::GetLayoutTypeByShaderType(eShader::Simple);
            DescriptorSet* descriptorSet = nullptr;
            if (mDescriptorAllocator->AllocateSet(layoutType, &descriptorSet) == false)
            {
                ASSERT(false, "Model 로드 중 DescriptorSet 구성 실패. filePath(%s)", filePath);
                return false;
            }
            std::vector<DescriptorWriteInfo> writeInfos;
            DescriptorLayout::GetLayoutWriteInfosByShader(layoutType, eShader::Simple, writeInfos);
            for(auto& writeInfo : writeInfos)
            {
                // MEMO: 통합 버퍼 생성 전까지 자신의 버퍼를 연결한다.
                if(writeInfo.bTexture && writeInfo.TextureType != eTextureType::TextureTypeCount)
                {
                    const Texture* const texture = mResourceManager->TryGetTexture(newSubMesh.Material.TextureHashes[static_cast<EnumInt>(writeInfo.TextureType)]);
                    ASSERT(texture != nullptr, "Model 로드 중 텍스처가 없습니다. textureType(%u), filePath(%s)", static_cast<EnumInt>(writeInfo.TextureType), filePath);
                    writeInfo.ImageView = texture->GetImageView();
                }
                else if(writeInfo.Descriptor.Type == VK_DESCRIPTOR_TYPE_SAMPLER)
                {
                    writeInfo.Sampler = mResourceManager->GetSamplerByType(eSamplerType::Linear);
                }
            }
            descriptorSet->UpdateDescriptorSet(writeInfos);
            descriptorSets.push_back(descriptorSet);

            layoutType = DescriptorLayout::GetLayoutTypeByShaderType(eShader::Shadow);
            DescriptorSet* shadowDescriptorSet = nullptr;
            if (mDescriptorAllocator->AllocateSet(layoutType, &shadowDescriptorSet) == false)
            {
                ASSERT(false, "Model 로드 중 DescriptorSet 구성 실패. filePath(%s)", filePath);
                return false;
            }
            writeInfos.clear();
            DescriptorLayout::GetLayoutWriteInfosByShader(layoutType, eShader::Shadow, writeInfos);
            for(auto& writeInfo : writeInfos)
            {
                // MEMO: 통합 버퍼 생성 전까지 자신의 버퍼를 연결한다.
                if(writeInfo.bTexture && writeInfo.TextureType != eTextureType::TextureTypeCount)
                {
                    const Texture* const texture = mResourceManager->TryGetTexture(newSubMesh.Material.TextureHashes[static_cast<EnumInt>(writeInfo.TextureType)]);
                    ASSERT(texture != nullptr, "Model 로드 중 텍스처가 없습니다. textureType(%u), filePath(%s)", static_cast<EnumInt>(writeInfo.TextureType), filePath);
                    writeInfo.ImageView = texture->GetImageView();
                }
                else if(writeInfo.Descriptor.Type == VK_DESCRIPTOR_TYPE_SAMPLER)
                {
                    writeInfo.Sampler = mResourceManager->GetSamplerByType(eSamplerType::Linear);
                }
            }

            shadowDescriptorSet->UpdateDescriptorSet(writeInfos);
            shadowDescriptorSets.push_back(shadowDescriptorSet);

            HashID mainPassPipelineHash;
            if (mRenderer->TryCreateGraphicsPipeline(mainPassPipelineHash, renderer::eShader::Simple, renderer::eVertexFormat::PTN, renderer::ePrimitiveTopology::Triangles, renderer::eRasterType::Basic, renderer::Opaque, renderer::eDepthStencilState::DepthOnMaskAllCompLessEqual, renderer::eRenderPass::Main) == false)
            {
                ASSERT(false, "Model 로드 중 파이프라인 구성 실패(main). filePath(%s)", filePath);
                return false;
            }
            newSubMesh.Pipelines.insert({ renderer::eRenderPass::Main, mainPassPipelineHash });

            HashID shadowPassPipelineHash;
            if (mRenderer->TryCreateGraphicsPipeline(shadowPassPipelineHash, renderer::eShader::Shadow, renderer::eVertexFormat::PTN, renderer::ePrimitiveTopology::Triangles, renderer::eRasterType::Basic, renderer::Opaque, renderer::eDepthStencilState::DepthOnMaskAllCompLessEqual, renderer::eRenderPass::Shadow) == false)
            {
                ASSERT(false, "Model 로드 중 파이프라인 구성 실패(shadow). filePath(%s)", filePath);
                return false;
            }
            newSubMesh.Pipelines.insert({ renderer::eRenderPass::Shadow, shadowPassPipelineHash });
            newMesh.SubMeshes.push_back(std::move(newSubMesh));
        }

        newMesh.VertexRange.StartIndex = newMesh.SubMeshes.front().VertexRange.StartIndex;
        newMesh.VertexRange.Count = totalVertexCount;

        newMesh.IndexRange.StartIndex = newMesh.SubMeshes.front().IndexRange.StartIndex;
        newMesh.IndexRange.Count = totalIndexCount;

        *outModel = new Model();
        (*outModel)->SetMesh(newMesh);
        (*outModel)->SetDescriptorSets(eShader::Simple, descriptorSets);
        (*outModel)->SetDescriptorSets(eShader::Shadow, shadowDescriptorSets);


        return true;
    }

    bool Loader::LoadSkybox(scene::Sky* const sky)
    {
        std::vector<VertexP> vertices;
        std::vector<uint32_t> indices;
        Mesh mesh;
        renderer::MeshGenerator::CreateSphere(10, 10, mesh, vertices, indices);

        ASSERT(mesh.SubMeshes.empty() == false, "skybox의 subMesh가 비어 있습니다. CreateSphere가 실패한 것 같습니다.");
        SubMesh& subMesh = mesh.SubMeshes.front();
        subMesh.Material.Factors.IsLitOn = static_cast<float>(false);
        mResourceManager->AddBufferData(vertices.data(), sizeof(VertexP), vertices.size(), mesh.MeshHash, subMesh.VertexRange, true);
        mResourceManager->AddBufferData(indices.data(), sizeof(uint32_t), indices.size(), mesh.MeshHash, subMesh.IndexRange, false);

        const int8_t* const filePath = reinterpret_cast<const int8_t*>("./AssetData/textures/skybox.dds");
        const HashID texHash = util::GetDjb2Hash(filePath);
        if (loadTexture(filePath, texHash, true) == false)
        {
            ASSERT(false, "Skybox 로드 중 텍스처 생성 실패. filePath(%s)", filePath);
            return false;
        }
        subMesh.Material.TextureHashes[static_cast<EnumInt>(eTextureType::Cube)] = texHash;

        HashID pipelineHash;
        if (mRenderer->TryCreateGraphicsPipeline(pipelineHash, eShader::Skybox, mesh.VertexFormat, ePrimitiveTopology::Triangles, eRasterType::Skybox, eBlendState::Opaque, eDepthStencilState::DepthOnMaskAllCompLessEqual, eRenderPass::Main) == false)
        {
            ASSERT(false, "Skybox 로드 중 pipeline 구성 실패. filePath(%s)", filePath);
            return false;
        }

        subMesh.Pipelines.insert({eRenderPass::Main, pipelineHash});

        // MEMO: configure DescriptorSet
        const eLayoutType layoutType = DescriptorLayout::GetLayoutTypeByShaderType(eShader::Skybox);
        std::vector<DescriptorSet*> descriptorSets;
        DescriptorSet* descriptorSet = nullptr;
        if (mDescriptorAllocator->AllocateSet(layoutType, &descriptorSet) == false)
        {
            ASSERT(false, "Skybox 로드 중 DescriptorSet 구성 실패. filePath(%s)", filePath);
            return false;
        }
        std::vector<DescriptorWriteInfo> writeInfos;
        DescriptorLayout::GetLayoutWriteInfosByShader(layoutType, eShader::Skybox, writeInfos);
        for (auto& writeInfo : writeInfos)
        {
            // MEMO: 통합 버퍼 생성 전까지 자신의 버퍼를 연결한다.
            if (writeInfo.bTexture && writeInfo.TextureType != eTextureType::TextureTypeCount)
            {
                const Texture* const texture = mResourceManager->TryGetTexture(subMesh.Material.TextureHashes[static_cast<EnumInt>(writeInfo.TextureType)]);
                ASSERT(texture != nullptr, "Skybox 로드 중 텍스처가 없습니다. textureType(%u), filePath(%s)", static_cast<EnumInt>(writeInfo.TextureType), filePath);
                writeInfo.ImageView = texture->GetImageView();
            }
            else if (writeInfo.Descriptor.Type == VK_DESCRIPTOR_TYPE_SAMPLER)
            {
                writeInfo.Sampler = mResourceManager->GetSamplerByType(eSamplerType::Linear);
            }
        }

        descriptorSet->UpdateDescriptorSet(writeInfos);
        descriptorSets.push_back(descriptorSet);
        sky->SetDescriptorSets(eShader::Skybox, descriptorSets);
        sky->SetMesh(mesh);
        return true;
    }

    bool Loader::LoadBillboard(scene::Billboard* const billboard)
    {
        std::vector<VertexPT> vertices;
        std::vector<uint32_t> indices;
        Mesh mesh;
        renderer::MeshGenerator::CreatePlane(mesh, vertices, indices);

        ASSERT(mesh.SubMeshes.empty() == false, "billboard의 subMesh가 비어 있습니다. CreateSphere가 실패한 것 같습니다.");
        SubMesh& subMesh = mesh.SubMeshes.front();
        subMesh.Material.Factors.IsLitOn = static_cast<float>(false);
        mResourceManager->AddBufferData(vertices.data(), sizeof(VertexPT), vertices.size(), mesh.MeshHash, subMesh.VertexRange, true);
        mResourceManager->AddBufferData(indices.data(), sizeof(uint32_t), indices.size(), mesh.MeshHash, subMesh.IndexRange, false);

        const int8_t* const filePath = reinterpret_cast<const int8_t*>("./AssetData/textures/lightIcon.png");
        const HashID texHash = util::GetDjb2Hash(filePath);
        if (loadTexture(filePath, texHash, false) == false)
        {
            ASSERT(false, "billboard 로드 중 텍스처 생성 실패. filePath(%s)", filePath);
            return false;
        }
        subMesh.Material.TextureHashes[static_cast<EnumInt>(eTextureType::Diffuse)] = texHash;


        HashID pipelineHash;
        if (mRenderer->TryCreateGraphicsPipeline(pipelineHash, eShader::Texture, mesh.VertexFormat, ePrimitiveTopology::TriangleStrip, eRasterType::Basic, eBlendState::AlphaBlend, eDepthStencilState::DepthOnMaskAllCompLessEqual, eRenderPass::Main) == false)
        {
            ASSERT(false, "billboard 로드 중 pipeline 구성 실패. filePath(%s)", filePath);
            return false;
        }

        subMesh.Pipelines.insert({ eRenderPass::Main, pipelineHash });


        // MEMO: configure DescriptorSet
        const eLayoutType layoutType = DescriptorLayout::GetLayoutTypeByShaderType(eShader::Texture);
        std::vector<DescriptorSet*> descriptorSets;
        DescriptorSet* descriptorSet = nullptr;
        if (mDescriptorAllocator->AllocateSet(layoutType, &descriptorSet) == false)
        {
            ASSERT(false, "billboard 로드 중 DescriptorSet 구성 실패. filePath(%s)", filePath);
            return false;
        }
        std::vector<DescriptorWriteInfo> writeInfos;
        DescriptorLayout::GetLayoutWriteInfosByShader(layoutType, eShader::Texture, writeInfos);
        for (auto& writeInfo : writeInfos)
        {
            // MEMO: 통합 버퍼 생성 전까지 자신의 버퍼를 연결한다.
            if (writeInfo.bTexture && writeInfo.TextureType != eTextureType::TextureTypeCount)
            {
                const Texture* const texture = mResourceManager->TryGetTexture(subMesh.Material.TextureHashes[static_cast<EnumInt>(writeInfo.TextureType)]);
                ASSERT(texture != nullptr, "billboard 로드 중 텍스처가 없습니다. textureType(%u), filePath(%s)", static_cast<EnumInt>(writeInfo.TextureType), filePath);
                writeInfo.ImageView = texture->GetImageView();
            }
            else if (writeInfo.Descriptor.Type == VK_DESCRIPTOR_TYPE_SAMPLER)
            {
                writeInfo.Sampler = mResourceManager->GetSamplerByType(eSamplerType::Linear);
            }
        }

        descriptorSet->UpdateDescriptorSet(writeInfos);
        descriptorSets.push_back(descriptorSet);
        billboard->SetDescriptorSets(eShader::Texture, descriptorSets);
        billboard->SetMesh(mesh);
        return true;
    }

    bool Loader::LoadFloor(scene::Floor* floor)
    {
        std::vector<VertexPTN> vertices;
        std::vector<uint32_t> indices;
        Mesh mesh;
        renderer::MeshGenerator::CreateGrid(XMFLOAT2(0.0f, 0.0f), 10, 10, 2, mesh, vertices, indices);

        ASSERT(mesh.SubMeshes.empty() == false, "floor의 subMesh가 비어 있습니다. CreateSphere가 실패한 것 같습니다.");
        SubMesh& subMesh = mesh.SubMeshes.front();
        subMesh.Material.Factors.Diffuse = XMFLOAT3(0.80f, 0.72f, 0.56f);
        subMesh.Material.Factors.Opacity = 1.0f;
        subMesh.Material.Factors.IsLitOn = static_cast<float>(true);
        subMesh.Material.Factors.Ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
        subMesh.Material.Factors.Shininess = 8;
        subMesh.Material.Factors.Specular = XMFLOAT3(0.16f, 0.144f, 0.112f);

        mResourceManager->AddBufferData(vertices.data(), sizeof(VertexPTN), vertices.size(), mesh.MeshHash, subMesh.VertexRange, true);
        mResourceManager->AddBufferData(indices.data(), sizeof(uint32_t), indices.size(), mesh.MeshHash, subMesh.IndexRange, false);

        HashID pipelineHash;
        if (mRenderer->TryCreateGraphicsPipeline(pipelineHash, eShader::Color, mesh.VertexFormat, ePrimitiveTopology::Triangles, eRasterType::Basic, eBlendState::Opaque, eDepthStencilState::DepthOnMaskAllCompLessEqual, eRenderPass::Main) == false)
        {
            ASSERT(false, "floor 로드 중 pipeline 구성 실패.");
            return false;
        }

        subMesh.Pipelines.insert({ eRenderPass::Main, pipelineHash });

        // MEMO: configure DescriptorSet
        const eLayoutType layoutType = DescriptorLayout::GetLayoutTypeByShaderType(eShader::Color);
        std::vector<DescriptorSet*> descriptorSets;
        DescriptorSet* descriptorSet = nullptr;
        if (mDescriptorAllocator->AllocateSet(layoutType, &descriptorSet) == false)
        {
            ASSERT(false, "billboard 로드 중 DescriptorSet 구성 실패.", );
            return false;
        }
        std::vector<DescriptorWriteInfo> writeInfos;
        DescriptorLayout::GetLayoutWriteInfosByShader(layoutType, eShader::Color, writeInfos);
        for (auto& writeInfo : writeInfos)
        {
            // MEMO: 통합 버퍼 생성 전까지 자신의 버퍼를 연결한다.
            if (writeInfo.bTexture && writeInfo.TextureType != eTextureType::TextureTypeCount)
            {
                const Texture* const texture = mResourceManager->TryGetTexture(subMesh.Material.TextureHashes[static_cast<EnumInt>(writeInfo.TextureType)]);
                ASSERT(texture != nullptr, "floor 로드 중 텍스처가 없습니다. textureType(%u)", static_cast<EnumInt>(writeInfo.TextureType));
                writeInfo.ImageView = texture->GetImageView();
            }
            else if (writeInfo.Descriptor.Type == VK_DESCRIPTOR_TYPE_SAMPLER)
            {
                writeInfo.Sampler = mResourceManager->GetSamplerByType(eSamplerType::Linear);
            }
        }

        descriptorSet->UpdateDescriptorSet(writeInfos);
        descriptorSets.push_back(descriptorSet);
        floor->SetDescriptorSets(eShader::Color, descriptorSets);
        floor->SetMesh(mesh);
        return true;
    }

    bool Loader::loadTexture(const int8_t* const filePath, HashID texHash, bool bIsDDS) const
    {
        if (mResourceManager->TryGetTexture(texHash) == nullptr)
        {
            Texture* texture = new Texture(mRenderer->GetDevice());
            if (texture->Create(filePath, bIsDDS) == false)
            {
                delete texture;
                return false;
            }
            mResourceManager->AddTexture(texHash, texture);
            texture = nullptr;
        }
        else
        {
            // TODO: 이 assert는 굳이 유지할 필요 없으므로 고도화 작업에서 제거 예정
            ASSERT(false, "텍스처가 두번 생성된 것 같습니다. 한번만 호출되어야 합니다. texHash(%u), filePath(%s)", texHash, filePath);
        }
        return true;
    }
}
