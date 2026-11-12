#pragma once
#include "../GfxPrerequisites.h"
#include "ShaderType.h"
#include "../../Util/Type.h"
#include "../Resources/VertexType.h"

namespace renderer
{
    struct DescriptorWriteInfo;
    class DescriptorSet;
    enum class eLayoutType : uint8_t;
    class Buffer;

    struct PoolInfo
    {
        uint16_t PoolIndex;
        eLayoutType LayoutType;
    };

    struct UniformWriteInfo
    {
        DescriptorSet* TargetSet;
        void* Data;
        BufferRange Range;
    };

    class DescriptorSet
    {
    public:
        DescriptorSet(const VkDevice& device, const std::vector<VkDescriptorSet>& descriptorSets, const PoolInfo& ownerInfo);
        ~DescriptorSet();

        bool Create();
        void UpdateDescriptorSet(const std::vector<DescriptorWriteInfo>& writeInfos);
        void UpdateDescriptorSet(const std::vector<DescriptorWriteInfo>& writeInfos, uint32_t inFlightIndex);
        void UpdateUniformData(const void* const pData, uint32_t count, uint32_t elementSize, uint32_t inFlightIndex);
        VkDescriptorSet GetDescriptorSet(uint32_t inFlightIndex) const;
        eLayoutType GetLayoutType() const;
    private:
        VkDevice mDevice;
        std::vector<Buffer*> mBuffers;
        std::vector<VkDescriptorSet> mDescriptorSets;
        PoolInfo mOwnerInfo;
    };
}

