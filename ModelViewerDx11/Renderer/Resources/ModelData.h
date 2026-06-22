#pragma once
#include "RenderTypes.h"
#include "TextureData.h"
#include "../../Util/Define.h"
#include "../../Util/Type.h"

namespace renderer
{
    struct Mesh
    {
        eInputLayout VertexLayoutType;
        HashID MeshHash;
        BufferRange VertexRange;
        BufferRange IndexRange;
        Material Material;
        HashID TextureHashes[static_cast<int8_t>(eTextureType::TextureTypeCount)];
        // for debugging
        int8_t MeshName[util::MAX_NAME_LENGTH];
    };
}
