#pragma once
#include "../GfxPrerequisites.h"
#include "../../Util/Type.h"

namespace renderer
{
    class ShaderModule;
    enum class eLayoutType : uint8_t;
    enum class eShader : uint8_t;

    class Shader
    {
    public:
        Shader(const VkDevice& device);
        ~Shader();

        bool Create(eShader shaderType, ShaderModule* const vs, ShaderModule* const ps);
        void Release();

        eShader GetShaderType() const;
        const ShaderModule* GetVs() const;
        const ShaderModule* GetPs() const;

        eLayoutType GetLayoutType() const;
    private:
        VkDevice mDevice;
        VkDescriptorSetLayout mDescriptorSetLayout;
        VkPipelineLayout mPipelineLayout;
        eLayoutType mLayoutType;
        ShaderModule* mVs;
        ShaderModule* mPs;
        eShader mShaderType;
    };
}
