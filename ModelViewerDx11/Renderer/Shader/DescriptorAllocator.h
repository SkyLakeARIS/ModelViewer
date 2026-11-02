#pragma once
#include "../GfxPrerequisites.h"

namespace renderer
{
    enum class eLayoutType : uint8_t;
    class DescriptorSet;

    class DescriptorAllocator
    {
    public:

        DescriptorAllocator(const VkDevice& device);
        ~DescriptorAllocator();

        bool AllocatePool(eLayoutType layoutType);
        bool AllocateSet(eLayoutType layoutType, DescriptorSet** outDescriptorSet);
    private:
        static constexpr uint16_t sMaxDescriptorCount = 64;
        static constexpr uint16_t sMaxDescriptorSetCount = 32;
    private:
        VkDevice mDevice;
        std::unordered_map<eLayoutType, std::vector<VkDescriptorPool>> mDescriptorPools;
        std::unordered_map<eLayoutType, uint16_t> mNowPoolIndices;
        // MEMO: 할당된 DescriptorSet 관리
        std::unordered_map<eLayoutType, std::vector<DescriptorSet*>> mDescriptorSetPools;
    };

}
