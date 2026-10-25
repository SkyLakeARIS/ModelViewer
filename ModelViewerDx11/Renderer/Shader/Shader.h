#pragma once
#include "../GfxPrerequisites.h"
#include "../../Util/Type.h"

namespace renderer
{
    enum class eShaderStageType;
    class ShaderModule;
    enum class eShader : uint8_t;

    struct DescriptorInfo
    {
        VkDescriptorType Type;
        uint32_t Count;
    };

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
        VkDescriptorSetLayout GetDescriptorSetLayout() const;
        VkPipelineLayout GetPipelineLayout() const;
        const std::vector<DescriptorInfo>& GetDescriptorInfos() const;
    private:
        VkDevice mDevice;
        VkDescriptorSetLayout mDescriptorSetLayout;
        VkPipelineLayout mPipelineLayout;
        ShaderModule* mVs;
        ShaderModule* mPs;
        eShader mShaderType;
        std::vector<DescriptorInfo> mDescriptorInfos;
    };
}
