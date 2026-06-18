#pragma once
#include "RenderTypes.h"
#include "TextureData.h"
#include "../../Util/Type.h"

namespace renderer
{
    // TODO: Generator 클래스를 만들면 옮겨야 함.
    static const int8_t* const VIRTUAL_ROOT_PATH = reinterpret_cast<const int8_t*>("/AssetData/Generated/");

    struct Vertex;

    struct Mesh
    {
        HashID MeshHash;
        BufferRange VertexRange;
        BufferRange IndexRange;
        Material Material;
        HashID TextureHashes[static_cast<int8_t>(eTextureType::TextureTypeCount)];
    };
}
