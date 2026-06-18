#include "ResourceManager.h"
#include "BufferManager.h"
#include "Model.h"
#include "TextureManager.h"
#include "../../Util/Macro.h"
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
        // TODO: 동기식이므로 받아오고 받아온 데이터를 각 Manager에 할당시키는 것으로 처리하면 될듯.
        // TODO: IndexList가 비어있을 수 있지 않을까 생각하면 Importer에서 좀 더 로직을 엄격하게 체크해야 할 것으로 보임.
        mModelImporter->LoadFbxModel(filePath, modelHash, modelContainer);

        std::vector<Mesh> meshes(modelContainer.Meshes.size());
        // TODO: 여기에서 로드한 Object를 처리해서 반환해 주는 게 좋을 것 같다.
        // 그리고 이곳에서 Imported 타입이 일단 Mesh 데이터로 바뀌는 시점.
        mBufferManager->AddVertexData(reinterpret_cast<int8_t*>(modelContainer.VertexBufferTotal.get()), sizeof(Vertex) * modelContainer.TotalVertexCount, modelContainer.ModelHash);

        mBufferManager->AddIndexData(reinterpret_cast<int8_t*>(modelContainer.IndexBufferTotal.get()), sizeof(uint32_t) * modelContainer.TotalIndexCount, modelContainer.ModelHash);

        auto meshIt = meshes.begin();
        int32_t vertexStartIndex = 0;
        int32_t indexStartIndex = 0;
        for(auto& mesh : modelContainer.Meshes)
        {
            meshIt->VertexRange.StartIndex = vertexStartIndex;
            meshIt->VertexRange.Count = mesh.VertexCount;
            vertexStartIndex += mesh.VertexCount;

            meshIt->IndexRange.StartIndex = indexStartIndex;
            meshIt->IndexRange.Count = mesh.IndexCount;
            indexStartIndex += mesh.IndexCount;

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
