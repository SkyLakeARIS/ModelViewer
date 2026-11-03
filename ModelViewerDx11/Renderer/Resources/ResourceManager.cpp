#include "ResourceManager.h"
#include "Sampler.h"
#include "Texture.h"
#include "../../Util/Macro.h"
#include "../Shader/Shader.h"
#include "../Shader/ShaderModule.h"
#include "../Shader/ShaderType.h"

namespace renderer
{
    ResourceManager::ResourceManager(VkDevice device)
        : mDevice(device)
    {
    }

    ResourceManager::~ResourceManager()
    {
        for (const auto& samplerIt : mSamplers)
        {
            delete samplerIt.second;
        }

        for (const auto& shaderIt : mShaders)
        {
            delete shaderIt;
        }
        mShaders.clear();

        for (const auto& shaderModuleIt : mShaderModules)
        {
            delete shaderModuleIt.second;
        }
        mShaderModules.clear();

        for(auto& texIt : mTextures)
        {
            delete texIt.second;
            texIt.second = nullptr;
        }
        mTextures.clear();

        for (auto& pipelineIt : mPipelines)
        {
            vkDestroyPipeline(mDevice, pipelineIt.second, nullptr);
        }
        mPipelines.clear();
    }

    bool ResourceManager::CreateShaders()
    {
        struct ShaderStageHash
        {
            eShader Type;
            HashID VertexShaderHash;
            HashID PixelShaderHash;
        };

        struct VertexShaderSourceInfo
        {
            eVertexShader Type;
            const char* SourcePath;
        };

        constexpr VertexShaderSourceInfo VertexShaderSourceList[] =
        {
            {eVertexShader::VsOutline, "Renderer/Shader/Precompiled/VsOutline.spv"},
            {eVertexShader::VsBasicWithShadow, "Renderer/Shader/Precompiled/VsBasicWithShadow.spv"},
            {eVertexShader::VsSimple, "Renderer/Shader/Precompiled/VsSimple.spv"},
            {eVertexShader::VsSkybox, "Renderer/Shader/Precompiled/VsSkybox.spv"},
            {eVertexShader::VsTexture, "Renderer/Shader/Precompiled/VsTexture.spv"},
            {eVertexShader::VsScreen, "Renderer/Shader/Precompiled/VsScreen.spv"},
            {eVertexShader::VsShadow, "Renderer/Shader/Precompiled/VsShadow.spv"},
            {eVertexShader::VsDeferred, "Renderer/Shader/Precompiled/VsDeferred.spv"},
            {eVertexShader::VsDebugColor, "Renderer/Shader/Precompiled/VsDebugColor.spv"},
        };
        static_assert(std::size(VertexShaderSourceList) == static_cast<uint8_t>(eVertexShader::VertexShaderCount), "열거형값과 List의 요소가 서로 일치해야 합니다.");


        struct PixelShaderSourceInfo
        {
            ePixelShader Type;
            const char* SourcePath;
        };

        constexpr PixelShaderSourceInfo PixelShaderSourceList[] =
        {
            {ePixelShader::PsOutline, "Renderer/Shader/Precompiled/PsOutline.spv"},
            {ePixelShader::PsBasicWithShadow, "Renderer/Shader/Precompiled/PsBasicWithShadow.spv"},
            {ePixelShader::PsShadow, "Renderer/Shader/Precompiled/PsShadow.spv"},
            {ePixelShader::PsTexture, "Renderer/Shader/Precompiled/PsTexture.spv"},
            {ePixelShader::PsSkybox, "Renderer/Shader/Precompiled/PsSkybox.spv"},
            {ePixelShader::PsColor, "Renderer/Shader/Precompiled/PsColor.spv"},
            {ePixelShader::PsDeferred, "Renderer/Shader/Precompiled/PsDeferred.spv"},
            {ePixelShader::PsTextureMRT, "Renderer/Shader/Precompiled/PsTextureMRT.spv"},
            {ePixelShader::PsSimple, "Renderer/Shader/Precompiled/PsSimple.spv"},
        };
        static_assert(std::size(PixelShaderSourceList) == static_cast<uint8_t>(ePixelShader::PixelShaderCount), "열거형값과 List의 요소가 서로 일치해야 합니다.");

        HashID vsHashes[static_cast<uint8_t>(eVertexShader::VertexShaderCount)];
        for (const VertexShaderSourceInfo shaderInfo : VertexShaderSourceList)
        {
            ShaderModule* shaderModule = new ShaderModule(mDevice);
            HashID shaderHash;
            if (shaderModule->Create(shaderInfo.SourcePath, eShaderStageType::Vertex, shaderHash) == false)
            {
                return false;
            }
            mShaderModules.insert({ shaderHash, shaderModule });
            vsHashes[static_cast<EnumInt>(shaderInfo.Type)] = shaderHash;
        }

        HashID psHashes[static_cast<uint8_t>(ePixelShader::PixelShaderCount)];
        for (const PixelShaderSourceInfo shaderInfo : PixelShaderSourceList)
        {
            ShaderModule* shaderModule = new ShaderModule(mDevice);
            HashID shaderHash;
            if (shaderModule->Create(shaderInfo.SourcePath, eShaderStageType::Pixel, shaderHash) == false)
            {
                return false;
            }
            mShaderModules.insert({ shaderHash, shaderModule });
            psHashes[static_cast<EnumInt>(shaderInfo.Type)] = shaderHash;
        }

        // construct shader mapping table
        const ShaderStageHash ShaderMapTable[] =
        {
            {eShader::Outline, vsHashes[static_cast<uint8_t>(eVertexShader::VsOutline)], psHashes[static_cast<uint8_t>(ePixelShader::PsOutline)]},
            {eShader::Skybox, vsHashes[static_cast<uint8_t>(eVertexShader::VsSkybox)], psHashes[static_cast<uint8_t>(ePixelShader::PsSkybox)]},
            { eShader::Shadow, vsHashes[static_cast<uint8_t>(eVertexShader::VsShadow)], psHashes[static_cast<uint8_t>(ePixelShader::PsShadow)]},
            {eShader::BasicWithShadow,  vsHashes[static_cast<uint8_t>(eVertexShader::VsBasicWithShadow)], psHashes[static_cast<uint8_t>(ePixelShader::PsBasicWithShadow)]},
            {eShader::Texture,  vsHashes[static_cast<uint8_t>(eVertexShader::VsTexture)], psHashes[static_cast<uint8_t>(ePixelShader::PsTextureMRT)]},
            {eShader::Color,  vsHashes[static_cast<uint8_t>(eVertexShader::VsSimple)], psHashes[static_cast<uint8_t>(ePixelShader::PsColor)]},
            {eShader::DebugHUD,  vsHashes[static_cast<uint8_t>(eVertexShader::VsScreen)], psHashes[static_cast<uint8_t>(ePixelShader::PsTexture)]},
            {eShader::Deferred,  vsHashes[static_cast<uint8_t>(eVertexShader::VsDeferred)], psHashes[static_cast<uint8_t>(ePixelShader::PsDeferred)]},
            {eShader::DebugColor,  vsHashes[static_cast<uint8_t>(eVertexShader::VsDebugColor)], psHashes[static_cast<uint8_t>(ePixelShader::PsColor)]},
            {eShader::Simple,  vsHashes[static_cast<uint8_t>(eVertexShader::VsSimple)], psHashes[static_cast<uint8_t>(ePixelShader::PsSimple)]},
        };
        static_assert(std::size(ShaderMapTable) == static_cast<uint8_t>(eShader::ShaderCount), "열거형값과 ShaderMapTable의 요소가 서로 일치해야 합니다.");

        for (const auto& shaderEntryIt : ShaderMapTable)
        {
            const auto& vsModule = mShaderModules.find(shaderEntryIt.VertexShaderHash);
            ASSERT(vsModule != mShaderModules.end(), "Vs Module이 제대로 초기화되지 않았습니다. type(%d)", static_cast<EnumInt>(shaderEntryIt.Type));
            const auto& psModule = mShaderModules.find(shaderEntryIt.PixelShaderHash);
            ASSERT(psModule != mShaderModules.end(), "Ps Module이 제대로 초기화되지 않았습니다. type(%d)", static_cast<EnumInt>(shaderEntryIt.Type));
            Shader* shader = new Shader(mDevice);
            if (shader->Create(shaderEntryIt.Type, vsModule->second, psModule->second) == false)
            {
                return false;
            }
            mShaders.push_back(shader);
        }

        return true;
    }

    bool ResourceManager::CreateSamplers()
    {
        for (uint32_t samplerType = 0; samplerType < static_cast<uint32_t>(eSamplerType::SamplerCount); ++samplerType)
        {
            Sampler* sampler = new Sampler(mDevice);
            if (sampler->Create(static_cast<eSamplerType>(samplerType)) == false)
            {
                return false;
            }

            mSamplers.insert({ static_cast<eSamplerType>(samplerType), sampler });
        }
        return true;
    }

    bool ResourceManager::TryAddTexture(HashID hash, Texture* const texture)
    {
        if(mTextures.find(hash) != mTextures.end())
        {
            return false;
        }

        mTextures.insert({ hash, texture });
        return true;
    }

    void ResourceManager::AddPipeline(HashID hash, VkPipeline pipeline)
    {
        ASSERT(mPipelines.find(hash) == mPipelines.end(), "pipeline 개체가 중복 삽입 되었습니다. 릭 가능성이 있습니다. hash(%u)", hash);
        mPipelines.insert({ hash, pipeline });
    }

    Texture* ResourceManager::TryGetTexture(HashID hash)
    {
        Texture* tex = nullptr;
        const auto& texIt = mTextures.find(hash);
        if (texIt != mTextures.end())
        {
            tex = texIt->second;
        }
        return tex;
    }

    Shader* ResourceManager::GetShaderByType(eShader type) const
    {
        return mShaders[static_cast<EnumInt>(type)];
    }

    Sampler* ResourceManager::GetSamplerByType(eSamplerType type) const
    {
        const auto& samplerIt = mSamplers.find(type);
        ASSERT(samplerIt != mSamplers.end(), "Sampler가 생성되지 않았거나 잘못된 type입니다. type(%u)", static_cast<EnumInt>(type));
        return samplerIt->second;
    }

    VkPipeline ResourceManager::TryGetPipeline(HashID hash) const
    {
        VkPipeline pipeline = nullptr;

        const auto& pipelineIt = mPipelines.find(hash);
        if(pipelineIt != mPipelines.end())
        {
            pipeline = pipelineIt->second;
        }
        return pipeline;
    }
}
