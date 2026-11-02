#include "DescriptorLayout.h"
#include "ShaderType.h"
#include "../Resources/TextureData.h"
#include "../../Util/Macro.h"
#include "../../Util/Type.h"

namespace renderer
{
    VkDevice DescriptorLayout::sDevice = {};
    std::unordered_map<eLayoutType, DescriptorLayout::LayoutPair> DescriptorLayout::sLayoutPairs = {};
    std::unordered_map<eLayoutType, std::vector<DescriptorInfo>> DescriptorLayout::sDescriptorInfos = {};

    bool DescriptorLayout::Initialize(VkDevice device)
    {
        ASSERT(sLayoutPairs.empty(), "Initialize가 두 번 호출되었습니다.");

        sDevice = device;

        for(EnumInt layoutType = 0; layoutType < static_cast<EnumInt>(eLayoutType::LayoutCount); ++layoutType)
        {
            std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
            switch (static_cast<eLayoutType>(layoutType))
            {
            case eLayoutType::Common:
            {
                // MEMO: 프레임 데이터(ViewProj, LightViewProj, Light, Camera, Ortho)를 UBO 하나로 모은다.
                layoutBindings =
                {
                    {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                };
                break;
            }
            case eLayoutType::Material:
            {
                layoutBindings =
                {
                    {7, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    {13, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    {15, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                };
                break;
            }
            case eLayoutType::Deferred:
            {
                // MEMO: 7~11 G-Buffer, 12 ShadowMap
                layoutBindings =
                {
                    {7, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    {8, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    {9, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    {10, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    {11, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    {12, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    {13, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                };
                break;
            }
            default:
            {
                ASSERT(false, "올바르지 않은 타입이거나 작업되지 않은 타입이 감지되었습니다. type(%u)", layoutType);
                return false;
            }
            }

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.pBindings = layoutBindings.data();
            descriptorSetLayoutCreateInfo.bindingCount = layoutBindings.size();

            LayoutPair& layoutPair = sLayoutPairs[static_cast<eLayoutType>(layoutType)];
            VkResult result = vkCreateDescriptorSetLayout(sDevice, &descriptorSetLayoutCreateInfo, nullptr, &layoutPair.SetLayout);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "DescriptorSetLayout 생성 실패. type(%u), result(%u)", layoutType, result);
                return false;
            }

            VkPushConstantRange pushConstantRange{};
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof(PushConstantBlock);
            pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

            // MEMO: set 0은 항상 Common, set 1은 각 레이아웃. Common은 열거형의 첫 값이라 먼저 만들어진다.
            std::vector<VkDescriptorSetLayout> setLayouts(1, sLayoutPairs[eLayoutType::Common].SetLayout);
            if(static_cast<eLayoutType>(layoutType) != eLayoutType::Common)
            {
                setLayouts.push_back(layoutPair.SetLayout);
            }

            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
            pipelineLayoutCreateInfo.setLayoutCount = setLayouts.size();
            pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
            pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

            result = vkCreatePipelineLayout(sDevice, &pipelineLayoutCreateInfo, nullptr, &layoutPair.PipelineLayout);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "PipelineLayout 생성 실패. type(%u), result(%u)", layoutType, result);
                return false;
            }

            std::vector<DescriptorInfo>& descriptorInfos = sDescriptorInfos[static_cast<eLayoutType>(layoutType)];
            descriptorInfos.reserve(layoutBindings.size());
            for(const auto& bindingInfo : layoutBindings)
            {
                descriptorInfos.push_back({bindingInfo.binding, bindingInfo.descriptorType, bindingInfo.descriptorCount});
            }
        }

        return true;
    }

    void DescriptorLayout::Release()
    {
        for(const auto& layoutPairIt : sLayoutPairs)
        {
            vkDestroyPipelineLayout(sDevice, layoutPairIt.second.PipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(sDevice, layoutPairIt.second.SetLayout, nullptr);
        }
        sLayoutPairs.clear();

        for(auto& descriptorInfosIt : sDescriptorInfos)
        {
            descriptorInfosIt.second.clear();
            std::vector<DescriptorInfo>().swap(descriptorInfosIt.second);
        }
        sDescriptorInfos.clear();
    }

    eLayoutType DescriptorLayout::GetLayoutTypeByShaderType(eShader shader)
    {
        eLayoutType layoutType = eLayoutType::LayoutCount;
        switch (shader)
        {
        case eShader::Outline:
        case eShader::Shadow:
        {
            layoutType = eLayoutType::Common;
            break;
        }
        case eShader::Skybox:
        case eShader::BasicWithShadow:
        case eShader::Texture:
        case eShader::Color:
        case eShader::DebugHUD:
        case eShader::DebugColor:
        case eShader::Simple:
        {
            layoutType = eLayoutType::Material;
            break;
        }
        case eShader::Deferred:
        {
            layoutType = eLayoutType::Deferred;
            break;
        }
        default:
        {
            ASSERT(false, "올바르지 않은 타입이거나 작업되지 않은 타입이 감지되었습니다. type(%u)", static_cast<EnumInt>(shader));
            break;
        }
        }
        return layoutType;
    }

    void DescriptorLayout::GetLayoutWriteInfos(eLayoutType layoutType, std::vector<DescriptorWriteInfo>& outWriteInfos)
    {
        ASSERT(outWriteInfos.empty(), "outWriteInfos는 비어있어야합니다.");

        const std::vector<DescriptorInfo>& descriptorInfos = GetDescriptorInfos(layoutType);
        outWriteInfos.reserve(descriptorInfos.size());
        for(const auto& descriptorInfo : descriptorInfos)
        {
            DescriptorWriteInfo writeInfo{};
            writeInfo.Descriptor = descriptorInfo;
            writeInfo.bTexture = (descriptorInfo.Type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
            writeInfo.TextureType = eTextureType::TextureTypeCount;
            outWriteInfos.push_back(writeInfo);
        }
    }

    void DescriptorLayout::GetLayoutWriteInfos(eLayoutType layoutType, eShader shaderType, std::vector<DescriptorWriteInfo>& outWriteInfos)
    {
        ASSERT(GetLayoutTypeByShaderType(shaderType) == layoutType, "shaderType과 맞지 않는 layoutType이 전달되었습니다. layoutType(%d), shaderType(%d)", static_cast<EnumInt>(layoutType), static_cast<EnumInt>(shaderType));

        GetLayoutWriteInfos(layoutType, outWriteInfos);

        struct TextureSlot
        {
            uint32_t Binding;
            eTextureType TextureType;
        };

        // MEMO: 바인딩 슬릇과 텍스처 타입으로 실제 리소스와 매칭
        std::vector<TextureSlot> textureSlots;
        switch (shaderType)
        {
        case eShader::Outline:
        case eShader::Shadow:
        case eShader::Color:
        case eShader::DebugColor:
        {
            // MEMO: 텍스처를 쓰지 않는 셰이더
            break;
        }
        case eShader::Skybox:
        {
            // TODO: eTextureType 정비 후 큐브맵 자리 지정
            break;
        }
        case eShader::BasicWithShadow:
        case eShader::Texture:
        case eShader::DebugHUD:
        case eShader::Simple:
        {
            textureSlots =
            {
                {7, eTextureType::Diffuse},
            };
            break;
        }
        case eShader::Deferred:
        {
            // TODO: 7~11 G-Buffer, 12 ShadowMap 자리 지정
            break;
        }
        default:
        {
            ASSERT(false, "올바르지 않은 타입이거나 작업되지 않은 타입이 감지되었습니다. type(%u)", static_cast<EnumInt>(shaderType));
            break;
        }
        }

        for(const auto& textureSlot : textureSlots)
        {
            bool bFound = false;
            for(auto& writeInfo : outWriteInfos)
            {
                if(writeInfo.Descriptor.Binding == textureSlot.Binding)
                {
                    ASSERT(writeInfo.bTexture, "TextureSlot의 binding이 텍스처 타입이 아닙니다. binding(%d)", textureSlot.Binding);
                    writeInfo.TextureType = textureSlot.TextureType;
                    bFound = true;
                    break;
                }
            }
            ASSERT(bFound, "TextureSlot의 binding이 레이아웃에 없습니다. layoutType(%d), binding(%d)", static_cast<EnumInt>(layoutType), textureSlot.Binding);
        }
    }

    const std::vector<DescriptorInfo>& DescriptorLayout::GetDescriptorInfos(eLayoutType type)
    {
        const auto& descriptorInfosIt = sDescriptorInfos.find(type);
        ASSERT(descriptorInfosIt != sDescriptorInfos.end(), "유효하지 않은 타입이거나 Initialize 전에 호출되었습니다. type(%u)", static_cast<EnumInt>(type));

        return descriptorInfosIt->second;
    }

    VkDescriptorSetLayout DescriptorLayout::GetDescriptorSetLayout(eLayoutType type)
    {
        const auto& layoutPairIt = sLayoutPairs.find(type);
        ASSERT(layoutPairIt != sLayoutPairs.end(), "유효하지 않은 타입이거나 Initialize 전에 호출되었습니다. type(%u)", static_cast<EnumInt>(type));

        return layoutPairIt->second.SetLayout;
    }

    VkPipelineLayout DescriptorLayout::GetPipelineLayout(eLayoutType type)
    {
        const auto& layoutPairIt = sLayoutPairs.find(type);
        ASSERT(layoutPairIt != sLayoutPairs.end(), "유효하지 않은 타입이거나 Initialize 전에 호출되었습니다. type(%u)", static_cast<EnumInt>(type));

        return layoutPairIt->second.PipelineLayout;
    }
}
