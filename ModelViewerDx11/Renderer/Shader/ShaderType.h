#pragma once
#include "../../Core/MathPrerequisites.h"

namespace renderer
{
    struct PushConstantBlock
    {
        XMMATRIX WorldMatrix;
    };

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
}
