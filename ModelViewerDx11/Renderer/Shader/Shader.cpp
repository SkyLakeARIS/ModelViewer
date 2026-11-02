#include "DescriptorLayout.h"
#include "Shader.h"
#include "ShaderType.h"
#include "../../Util/Macro.h"

namespace renderer
{

    Shader::Shader(const VkDevice& device)
        : mDevice(device)
        , mDescriptorSetLayout(nullptr)
        , mPipelineLayout(nullptr)
        , mLayoutType()
        , mVs(nullptr)
        , mPs(nullptr)
        , mShaderType()
    {
    }

    Shader::~Shader()
    {
        mVs = nullptr;
        mPs = nullptr;
        vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
    }

    bool Shader::Create(eShader shaderType, ShaderModule* const vs, ShaderModule* const ps)
    {
        mShaderType = shaderType;

        mVs = vs;
        mPs = ps;

        mLayoutType = DescriptorLayout::GetLayoutTypeByShaderType(shaderType);
        return true;
    }

    void Shader::Release()
    {
        // MEMO: Pooling 시에 활용할 리셋 함수
    }

    eShader Shader::GetShaderType() const
    {
        return mShaderType;
    }

    const ShaderModule* Shader::GetVs() const
    {
        return mVs;
    }

    const ShaderModule* Shader::GetPs() const
    {
        return mPs;
    }

    eLayoutType Shader::GetLayoutType() const
    {
        return mLayoutType;
    }
}
