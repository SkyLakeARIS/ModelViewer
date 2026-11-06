#pragma once
#include "../GfxPrerequisites.h"
#include "../../Util/Type.h"

namespace renderer
{
    enum class eShaderStageType
    {
        Vertex,
        Pixel
    };

    enum class eVertexShader : uint32_t
    {
        VsBasicWithShadow,
        VsOutline,
        VsTexture,
        VsSimple,
        VsSkybox,
        VsScreen,
        VsShadow,
        VsDeferred,
        VsDebugColor,
        VertexShaderCount
    };

    enum class ePixelShader : uint32_t
    {
        PsBasicWithShadow,
        PsOutline,
        PsShadow,
        PsSkybox,
        PsSkyboxMRT,
        PsTexture,
        PsColor,
        PsDeferred,
        PsTextureMRT,
        PsSimple,
        PixelShaderCount
    };

    class ShaderModule
    {
    public:
        ShaderModule(const VkDevice& device);
        ~ShaderModule();

        bool Create(const char* const path, eShaderStageType stageType, HashID& outShaderStageHash);

        eShaderStageType GetShaderStageType() const;
        VkShaderModule GetShaderModule() const;
    public:
        static const char* const sEntryPoint;
    private:
        VkDevice mDevice;
        VkShaderModule mShaderModule;
        eShaderStageType mStageType;
    };
}

