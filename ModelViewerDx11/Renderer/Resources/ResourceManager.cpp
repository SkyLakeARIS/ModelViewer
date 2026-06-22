#include "ResourceManager.h"
#include "BufferManager.h"
#include "Model.h"
#include "TextureManager.h"
#include "../../Util/Macro.h"
#include "../../Util/Util.h"
#include "../Importer/ImportedModelData.h"
#include "../Importer/ModelImporter.h"

namespace renderer
{
    ResourceManager::ResourceManager(ID3D11Device* const device, TextureManager* const textureManager, ModelImporter* const importer, BufferManager* const bufferManager)
        : mDevice(device)
        , mTextureManager(textureManager)
        , mModelImporter(importer)
        , mBufferManager(bufferManager)
    {
        ASSERT(device != nullptr, "device is nullptr");
        ASSERT(textureManager != nullptr, "textureManager is nullptr");
        ASSERT(importer != nullptr, "importer is nullptr");
        ASSERT(bufferManager != nullptr, "bufferManager is nullptr");
    }

    ResourceManager::~ResourceManager()
    {
        mDevice = nullptr;
        mTextureManager = nullptr;
        mModelImporter = nullptr;
        mBufferManager = nullptr;
    }

    void ResourceManager::LoadModel(const int8_t* const filePath, Model* const outModel)
    {
        ASSERT(filePath != nullptr, "invalid path. (filePath is nullptr) ");
        ASSERT(*filePath != ' ', "invalid path. (filePath may empty string) ");
        ASSERT(*filePath != '\0', "invalid path. (filePath may empty(null) string) ");

        HashID modelHash = 0;
        ImportedModelContainer modelContainer;
        // TODO: IndexList가 비어있을 수 있지 않을까 생각하면 Importer에서 좀 더 로직을 엄격하게 체크해야 할 것으로 보임.
        mModelImporter->LoadFbxModel(filePath, modelHash, modelContainer);

        std::vector<Mesh> meshes(modelContainer.Meshes.size());

        auto meshIt = meshes.begin();
        for(auto& mesh : modelContainer.Meshes)
        {
            meshIt->VertexLayoutType = eInputLayout::PTN;
            const int16_t strideVertex = GetVertexStrideSize(meshIt->VertexLayoutType);
            const int16_t strideIndex = mBufferManager->GetIndexStrideSize();
            meshIt->MeshHash = util::GetDjb2Hash(mesh.MeshName);
            mBufferManager->AddVertexData(reinterpret_cast<int8_t*>(mesh.VertexBuffer.get()), strideVertex * mesh.VertexCount, meshIt->MeshHash, strideVertex, meshIt->VertexRange);

            mBufferManager->AddIndexData(reinterpret_cast<int8_t*>(mesh.IndexBuffer.get()), strideIndex * mesh.IndexCount, meshIt->MeshHash, strideIndex, meshIt->IndexRange);

            memcpy(meshIt->MeshName, mesh.MeshName, util::MAX_NAME_LENGTH);

            meshIt->Material = std::move(mesh.Material);

            for (int32_t tex = 0; tex < static_cast<int32_t>(eTextureType::TextureTypeCount); ++tex)
            {
                if(mesh.Textures[tex].TextureHash)
                {
                    mTextureManager->AddTexture(mesh.Textures[tex].FilePath, mesh.Textures[tex].TextureHash);
                    meshIt->TextureHashes[tex] = mesh.Textures[tex].TextureHash;
                }
            }
            ++meshIt;
        }

        outModel->SetMeshes(meshes);
        outModel->SetCenterPoint(modelContainer.CenterPoint);
    }

}
