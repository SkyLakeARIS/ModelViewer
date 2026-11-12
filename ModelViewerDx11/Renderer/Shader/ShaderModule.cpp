#include "ShaderModule.h"
#include <fstream>
#include "../../Util/Macro.h"
#include "../../Util/Util.h"

namespace renderer
{
    const char* const ShaderModule::sEntryPoint = "main";

    ShaderModule::ShaderModule(const VkDevice& device)
        : mDevice(device)
        , mShaderModule(nullptr)
        , mStageType()
    {
    }

    ShaderModule::~ShaderModule()
    {
        vkDestroyShaderModule(mDevice, mShaderModule, nullptr);
    }

    bool ShaderModule::Create(const char* const path, eShaderStageType stageType, HashID& outShaderStageHash)
    {
        ASSERT(path, "path는 유효해야합니다. path(%s)", path);

        std::ifstream shaderFile(path, std::ios::ate | std::ios::binary);

        if (shaderFile.is_open() == false)
        {
            ASSERT(false, "셰이더 파일 로드 실패 ");
            return false;
        }

        const uint32_t fileSize = shaderFile.tellg();
        shaderFile.seekg(0, std::ios::beg);

        std::vector<uint8_t> shaderCode(fileSize);

        shaderFile.read(reinterpret_cast<char*>(shaderCode.data()), fileSize);

        // make shaderModule
        VkShaderModuleCreateInfo shaderModuleCreateInfo{};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = fileSize;
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

        const VkResult result = vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &mShaderModule);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "ShaderModule 생성 실패 resultCode(%d)", result);
            return false;
        }
        // make shaderStgae
        mStageType = stageType;
        outShaderStageHash = util::GetDjb2Hash(reinterpret_cast<const int8_t*>(path));
        return true;
    }

    eShaderStageType ShaderModule::GetShaderStageType() const
    {
        return mStageType;
    }

    VkShaderModule ShaderModule::GetShaderModule() const
    {
        return mShaderModule;
    }
}
