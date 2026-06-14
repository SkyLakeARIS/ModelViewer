#pragma once
#include "../../framework.h"
#include "../../Util/Define.h"
#include "../Resources/RenderTypes.h"
#include "../Resources/TextureData.h"

namespace renderer
{
    struct Vertex;

    struct ImportedTextureData
    {
        int8_t FilePath[util::MAX_PATH_LENGTH];
        HashID TextureHash;
        eTextureType TextureType;
    };

    // MEMO:각 mesh에 대한 해시는 현재 필요없으므로 추가하지 않았음.(fbx에 mesh에 대한 이름은 존재하는 것으로 기억)
    struct ImportedMeshData
    {
        uint32_t VertexCount;
        uint32_t IndexCount;
        Material Material;
        ImportedTextureData Textures[static_cast<int32_t>(eTextureType::TextureTypeCount)];
        // TODO: 나중에 DebugTag로 이름을 추적할 수 있게 저장할 수 있도록 하면 어떨지.(별로도 관리하는 클래스나, 구조체에 추가하거나.)
    };

    struct ImportedModelContainer
    {
        HashID ModelHash;
        std::unique_ptr<Vertex[]> VertexBufferTotal;
        int32_t TotalVertexCount;
        std::unique_ptr<uint32_t[]> IndexBufferTotal;
        int32_t TotalIndexCount;
        XMFLOAT4 CenterPoint;
        std::vector<ImportedMeshData> Meshes;
    };
}
