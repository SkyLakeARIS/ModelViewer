#include "DescriptorSet.h"
#include "../VkHelper.h"
#include "../../Util/Macro.h"
#include "../Resources/Buffer.h"
#include "../Resources/RenderTypes.h"
#include "../Shader/Shader.h"

namespace renderer
{
    DescriptorSet::DescriptorSet(const VkDevice& device, const std::vector<VkDescriptorSet>& descriptorSets, const PoolInfo& ownerInfo)
        : mDevice(device)
        , mDescriptorSets(descriptorSets)
        , mOwnerInfo(ownerInfo)
    {
    }

    DescriptorSet::~DescriptorSet()
    {
        for(const auto& buffer : mBuffers)
        {
            delete buffer;
        }
        mBuffers.clear();
        std::vector<Buffer*>().swap(mBuffers);
    }

    bool DescriptorSet::Create(uint32_t frameOverlap)
    {
        mBuffers.reserve(frameOverlap);
        for(uint32_t inFlightCount = 0; inFlightCount < frameOverlap; ++inFlightCount)
        {
            Buffer* buffer = new Buffer(mDevice);
            if(buffer->Create(512, Buffer::eBufferType::Uniform, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), VkHelper::GetGraphicsQueueFamilyIndex()) == false)
            {
                return false;
            }
            mBuffers.push_back(buffer);
        }
        return true;
    }

    void DescriptorSet::UpdateDescriptorSet(eShader shader)
    {
        const uint32_t inFlightCount = VkHelper::GetFrameInFlightCount();
        for (uint32_t inFlightIndex = 0; inFlightIndex < inFlightCount; ++inFlightIndex)
        {
            switch (shader)
            {
            case eShader::Outline:
            {
                //VkDescriptorBufferInfo bufferInfoPerFrame{};
                //bufferInfoPerFrame.buffer = mUbPerFrames[inFlightIndex].Buffer;
                //bufferInfoPerFrame.offset = inFlightIndex * sizeof(CbViewProj);
                //bufferInfoPerFrame.range = sizeof(CbViewProj);

                //VkDescriptorBufferInfo bufferInfoPerObject{};
                //bufferInfoPerObject.buffer = mUbPerObjects[inFlightIndex].Buffer;
                //bufferInfoPerObject.offset = inFlightIndex * sizeof(CbWorld);
                //bufferInfoPerObject.range = sizeof(CbWorld);

                //const std::vector<VkWriteDescriptorSet> WriteDescriptors =
                //{
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfoPerObject, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfoPerFrame, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 6, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfoPerFrame, {}},
                //};

                //vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);

                break;
            }
            case eShader::Skybox:
            {
                //VkDescriptorBufferInfo bufferInfo{};
                //bufferInfo.buffer = mUbPerFrames[frameIndex].Buffer;
                //bufferInfo.offset = 0;
                //bufferInfo.range = 0;

                //VkDescriptorImageInfo imageInfo{};
                //imageInfo.imageLayout = {};
                //imageInfo.imageView = 0;
                //imageInfo.sampler = {};

                //VkWriteDescriptorSet writeDescriptorSet{};
                //writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                //writeDescriptorSet.dstArrayElement = 0; // 배열 업데이트 시작 인덱스;
                //writeDescriptorSet.descriptorCount = 1; // 업데이트 요소 수;
                //writeDescriptorSet.dstSet = mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0];
                //writeDescriptorSet.dstBinding = // 업데이트 할 binding slot;
                //    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                //writeDescriptorSet.pBufferInfo = &bufferInfo;
    /*
                const std::vector<VkWriteDescriptorSet> WriteDescriptors =
                {
                    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 7, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 13, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, {}, {}},
                    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 15, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &imageInfo, {}, {}},
                };

                vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
     */           break;
            }
            case eShader::Shadow:
            {
                /*           const std::vector<VkWriteDescriptorSet> WriteDescriptors =
                           {
                               {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                               {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 2, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                           };

                           vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);*/
                break;
            }
            case eShader::BasicWithShadow:
            {
                //const std::vector<VkWriteDescriptorSet> WriteDescriptors =
                //{
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 2, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 3, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 4, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},

                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 7, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 13, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, {}, {}},

                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 15, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &imageInfo, {}, {}},
                //};

                //vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
                break;
            }
            case eShader::Texture:
            {
                //const std::vector<VkWriteDescriptorSet> WriteDescriptors =
                //{
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},

                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 7, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 13, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, {}, {}},
                //};

                //vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
                break;
            }
            case eShader::Color:
            {
                //const std::vector<VkWriteDescriptorSet> WriteDescriptors =
                //{
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},

                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 15, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &imageInfo, {}, {}},
                //};

                //vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
                break;
            }
            case eShader::DebugHUD:
            {
                //const std::vector<VkWriteDescriptorSet> WriteDescriptors =
                //{
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 5, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 7, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 13, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, {}, {}},
                //};

                //vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
                break;
            }
            case eShader::Deferred:
            {
                //const std::vector<VkWriteDescriptorSet> WriteDescriptors =
                //{
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 2, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 3, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 4, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},

                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 7, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 8, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 9, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 10, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 11, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 12, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},

                //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 13, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, {}, {}},
                //};

                //vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
                break;
            }
            case eShader::DebugColor:
            {
                //    const std::vector<VkWriteDescriptorSet> WriteDescriptors =
                //    {
                //        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                //        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},

                //        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 15, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &imageInfo, {}, {}},
                //    };

                //    vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
                break;
            }
            case eShader::Simple:
            {
                VkDescriptorBufferInfo bufferInfoPerFrame{};
                bufferInfoPerFrame.buffer = mBuffers[inFlightIndex]->GetBuffer();
                bufferInfoPerFrame.offset = 0;
                bufferInfoPerFrame.range = sizeof(CbViewProj);

                const std::vector<VkWriteDescriptorSet> WriteDescriptors =
                {
                    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mDescriptorSets[inFlightIndex], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfoPerFrame, {}},
                };

                vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
                break;
            }
            default:
                ASSERT(false, "올바르지 않은 타입이 전달되었습니다. type(%u)", static_cast<uint8_t>(shader));
                break;
            }
        }
    }

    void DescriptorSet::UpdateUniformData(const void* const pData, uint32_t count, uint32_t elementSize, uint32_t inFlightIndex, bool bPerFrame)
    {
        const uint32_t offset = bPerFrame ? 0 : sizeof(CbViewProj);
        mBuffers[inFlightIndex]->AddDataOffset(pData, count, elementSize, offset);
    }

    VkDescriptorSet DescriptorSet::GetDescriptorSet(uint32_t inFlightIndex) const
    {
        return mDescriptorSets[inFlightIndex];
    }

    void DescriptorSet::GetLayoutWriteInfos(eShader shaderType, std::vector<TextureWriteInfo>& outTextureWriteInfos)
    {
        switch(shaderType)
        {
        case eShader::Outline:
        {
            break;
        }
        case eShader::Skybox:
        {
            break;
        }
        case eShader::Shadow:
        {
            break;
        }
        case eShader::BasicWithShadow:
        {
            break;
        }
        case eShader::Texture:
        {
            break;
        }
        case eShader::Color:
        {
            break;
        }
        case eShader::DebugHUD:
        {
            break;
        }
        case eShader::Deferred:
        {
            break;
        }
        case eShader::DebugColor:
        {
            break;
        }
        case eShader::Simple:
        {

            //VkDescriptorBufferInfo bufferInfoPerObject{};
            //bufferInfoPerObject.buffer = mBuffers[inFlightIndex]->GetBuffer();
            //bufferInfoPerObject.offset = sizeof(CbViewProj);
            //bufferInfoPerObject.range = sizeof(CbWorld);

            //const std::vector<VkWriteDescriptorSet> WriteDescriptors =
            //{
            //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mDescriptorSets[inFlightIndex], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfoPerObject, {}},
            //    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mDescriptorSets[inFlightIndex], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfoPerFrame, {}},
            //};
            //for(uint32_t)
            //{
            //    
            //}
            break;
        }
        default:
        {
            break;
        }
        }
    }
}
