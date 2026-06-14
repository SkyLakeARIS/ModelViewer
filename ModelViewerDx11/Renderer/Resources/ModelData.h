#pragma once
#include "RenderTypes.h"
#include "TextureData.h"
#include "../../Util/Type.h"

namespace renderer
{
    // TODO: Generator 클래스를 만들면 옮겨야 함.
    static const int8_t* const VIRTUAL_ROOT_PATH = reinterpret_cast<const int8_t*>("/AssetData/Generated/");

    struct Vertex;

    constexpr size_t MESH_NAME_LENGTH = 128;

    struct Mesh
    {
        WCHAR Name[MESH_NAME_LENGTH];
        std::vector<Vertex> Vertex;
        std::vector<unsigned int> IndexList;
        Material Material;
        bool bLightMap;
        /*
         *  TODO 현재 구조는 같은 파일을 여러번 로드해서 가지고 있기 때문에
         * 나중에 TextureManager가 가지고 있어야 할 듯. ( ID-SRV, ID - 이름을 해시로 or  번호)
         * 셰이더랑 버텍스 구조 변경해야 할 수도.
         */
        ID3D11ShaderResourceView* Texture;
        ID3D11ShaderResourceView* TextureNormal;
        uint8 NumTexuture;
    };

    struct MeshNew
    {
        HashID MeshHash;
        BufferRange VertexRange;
        BufferRange IndexRange;
        Material Material;
        HashID TextureHashes[static_cast<int8_t>(eTextureType::TextureTypeCount)];
    };
}
