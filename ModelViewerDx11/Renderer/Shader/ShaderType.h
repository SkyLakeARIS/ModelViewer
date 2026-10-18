#pragma once
#include "../../Util/Type.h"

namespace renderer
{
    enum class eCbType : uint8_t
    {
        CbWorld,
        CbViewProj,
        CbLightViewProjMatrix,
        CbCameraPosition,
        CbOutlineProperty,
        CbLightProperty,
        CbMaterialFactors,
        CbColor,
        CbOrthoMatrix,
        ConstantBufferCount
    };

    enum class eShader : uint8_t
    {
        Outline,
        Skybox,
        Shadow,
        BasicWithShadow,
        Texture,
        Color,
        DebugHUD,
        Deferred,
        DebugColor,
        Simple,
        ShaderCount
    };

    struct MaterialCbBinding
    {
        eCbType Type;
        bool bBindPixelShader;
        int8_t BindSlot;
    };
}
