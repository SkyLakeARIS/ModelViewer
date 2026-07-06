#pragma once
#include "../../framework.h"
#include "../../Util/Define.h"
#include "../Resources/RenderTypes.h"
#include "../Resources/TextureData.h"

namespace renderer
{
    struct VertexPTN;

    struct ImportedTextureData
    {
        int8_t FilePath[util::MAX_PATH_LENGTH];
        HashID TextureHash;
        eTextureType TextureType;
    };

    struct ImportedMeshData
    {
        int8_t MeshName[util::MAX_NAME_LENGTH];
        std::unique_ptr<VertexPTN[]> VertexBuffer;
        uint32_t VertexCount;
        std::unique_ptr<uint32_t[]> IndexBuffer;
        uint32_t IndexCount;
        Material Material;
        ImportedTextureData Textures[static_cast<int32_t>(eTextureType::TextureTypeCount)];
    };

    struct ImportedModelContainer
    {
        HashID ModelHash;
        XMFLOAT4 CenterPoint;
        std::vector<ImportedMeshData> Meshes;
    };
}
