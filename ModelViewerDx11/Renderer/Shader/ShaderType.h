#pragma once
#include "../../Core/MathPrerequisites.h"
#include "../Resources/Material.h"
namespace renderer
{
    struct PushConstantBlock
    {
        XMMATRIX WorldMatrix;
        MaterialFactors MaterialFactors;
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
        ShaderCount
    };
}
