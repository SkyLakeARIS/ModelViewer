#pragma once
#include "../../Util/Type.h"

namespace renderer
{
    enum class eTextureType : uint8_t
    {
        Diffuse,
        Normal,
        Shadow,
        GBufferColor,
        GBufferNormal,
        GBufferPosition,
        GBufferSpecular,
        GBufferAmbient,
        TextureTypeCount,
    };
}
