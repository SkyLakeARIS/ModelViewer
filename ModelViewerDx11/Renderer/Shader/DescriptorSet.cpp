#include "DescriptorSet.h"
#include "DescriptorLayout.h"
#include "../VkHelper.h"
#include "../../Util/Macro.h"
#include "../Resources/Buffer.h"
#include "../Resources/RenderTypes.h"
#include "../Resources/Sampler.h"

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

    bool DescriptorSet::Create()
    {
        const uint32_t inFlightCount = VkHelper::GetFrameInFlightCount();
        mBuffers.reserve(inFlightCount);
        for(uint32_t index = 0; index < inFlightCount; ++index)
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

    void DescriptorSet::UpdateDescriptorSet(const std::vector<DescriptorWriteInfo>& writeInfos)
    {
        const uint32_t inFlightCount = VkHelper::GetFrameInFlightCount();
        for(uint32_t inFlightIndex = 0; inFlightIndex < inFlightCount; ++inFlightIndex)
        {
            UpdateDescriptorSet(writeInfos, inFlightIndex);
        }
    }

    void DescriptorSet::UpdateDescriptorSet(const std::vector<DescriptorWriteInfo>& writeInfos, uint32_t inFlightIndex)
    {
        ASSERT(inFlightIndex < mDescriptorSets.size(), "inFlightIndex가 범위를 벗어났습니다. inFlightIndex(%u), descriptorSetCount(%u)", inFlightIndex, static_cast<uint32_t>(mDescriptorSets.size()));
        ASSERT(writeInfos.empty() == false, "비어있는 배열이 전달되었습니다. 의도한지 체크하세요. layoutType(%u)", mOwnerInfo.LayoutType);
        const std::vector<DescriptorInfo>& descriptorInfos = DescriptorLayout::GetDescriptorInfos(mOwnerInfo.LayoutType);
        ASSERT(writeInfos.size() == descriptorInfos.size(), "writeInfos 개수가 레이아웃과 다릅니다. layoutType(%d), writeInfoCount(%d), layoutCount(%d)", static_cast<EnumInt>(mOwnerInfo.LayoutType), writeInfos.size(), descriptorInfos.size());

        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkWriteDescriptorSet> writeDescriptorSets;
        imageInfos.reserve(writeInfos.size());
        bufferInfos.reserve(writeInfos.size());
        writeDescriptorSets.reserve(writeInfos.size());

        for(const auto& writeInfo : writeInfos)
        {
            ASSERT(writeInfo.Descriptor.Count == 1, "배열 바인딩은 아직 지원하지 않습니다. (Count > 1) binding(%d), count(%d)", writeInfo.Descriptor.Binding, writeInfo.Descriptor.Count);

            if(writeInfo.bTexture && writeInfo.ImageView == nullptr)
            {
                continue;
            }

            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = mDescriptorSets[inFlightIndex];
            writeDescriptorSet.dstBinding = writeInfo.Descriptor.Binding;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.descriptorCount = writeInfo.Descriptor.Count;
            writeDescriptorSet.descriptorType = writeInfo.Descriptor.Type;

            switch (writeInfo.Descriptor.Type)
            {
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            {
                imageInfos.push_back({nullptr, writeInfo.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
                writeDescriptorSet.pImageInfo = &imageInfos.back();
                break;
            }
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            {
                ASSERT(writeInfo.Sampler != nullptr, "Sampler 정보가 세팅되지 않았습니다. binding(%d)", writeInfo.Descriptor.Binding);
                imageInfos.push_back({writeInfo.Sampler->GetSampler(), nullptr, VK_IMAGE_LAYOUT_UNDEFINED});
                writeDescriptorSet.pImageInfo = &imageInfos.back();
                break;
            }
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            {
                // MEMO: 통합 버퍼 전까지는 넘어온 버퍼가 없으면 자기 버퍼를 연결한다.
                if(writeInfo.Buffer == nullptr)
                {
                    bufferInfos.push_back({mBuffers[inFlightIndex]->GetBuffer(), 0, VK_WHOLE_SIZE});
                }
                else
                {
                    bufferInfos.push_back({writeInfo.Buffer->GetBuffer(), writeInfo.Offset, writeInfo.Range});
                }
                writeDescriptorSet.pBufferInfo = &bufferInfos.back();
                break;
            }
            default:
            {
                ASSERT(false, "처리되지 않은 DescriptorType이 전달되었습니다. type(%d)", writeInfo.Descriptor.Type);
                continue;
            }
            }

            writeDescriptorSets.push_back(writeDescriptorSet);
        }

        vkUpdateDescriptorSets(mDevice, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }

    void DescriptorSet::UpdateUniformData(const void* const pData, uint32_t count, uint32_t elementSize, uint32_t inFlightIndex)
    {
        mBuffers[inFlightIndex]->AddDataOffset(pData, count, elementSize, 0);
    }

    VkDescriptorSet DescriptorSet::GetDescriptorSet(uint32_t inFlightIndex) const
    {
        return mDescriptorSets[inFlightIndex];
    }

    eLayoutType DescriptorSet::GetLayoutType() const
    {
        return mOwnerInfo.LayoutType;
    }
}
