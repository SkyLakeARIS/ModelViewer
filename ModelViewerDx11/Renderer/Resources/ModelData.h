#pragma once
#include "RenderTypes.h"
#include "../../Util/Type.h"

namespace renderer
{
    // TODO: Generator 클래스를 만들면 옮겨야 함.
    static const int8_t* const VIRTUAL_ROOT_PATH = reinterpret_cast<const int8_t*>("/AssetData/Generated/");

    struct Vertex;

    constexpr size_t MESH_NAME_LENGTH = 128;

    struct Mesh
    {
        /*
         * MEMO vertex/index list는 하나로 관리하고 mesh는 각 vertex, index에 대해서 offset만 가지도록 변경 (하면 또 대규모 공사인데)
         * -> draw 로직에 큰 변화는 없음. 하지만 모델에서 임포터에서 받은 메시데이터를 다시 하나로 뭉치는 작업을 임포터로 옮길 수 있음.
         * -> 근데 그 뿐이기 때문에 좀 더 고민
         */
        WCHAR Name[MESH_NAME_LENGTH];
        std::vector<Vertex> Vertex;
        std::vector<unsigned int> IndexList;
        Material Material;
        bool bLightMap;
        /*
         *  MEMO 현재 구조는 같은 파일을 여러번 로드해서 가지고 있기 때문에
         * 나중에 TextureManager가 가지고 있어야 할 듯. ( ID-SRV, ID - 이름을 해시로 or  번호)
         * 셰이더랑 버텍스 구조 변경해야 할 수도.
         */
        ID3D11ShaderResourceView* Texture;
        ID3D11ShaderResourceView* TextureNormal;
        uint8 NumTexuture;
    };
}