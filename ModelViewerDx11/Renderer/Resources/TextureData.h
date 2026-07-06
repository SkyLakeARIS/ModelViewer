#pragma once
#include "../../framework.h"

namespace renderer
{
    enum class eTextureType
    {
        Diffuse,
        Normal,
        TextureTypeCount,
    };

    struct TextureData
    {
        ID3D11ShaderResourceView* SRV;
        HashID Hash;
    };
}
