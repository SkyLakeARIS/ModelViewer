#pragma once
#include "../GfxPrerequisites.h"
#include "../../Util/Type.h"

namespace renderer
{
    class DescriptorSet;
    class Shader;
    struct DescriptorInfo;

    class DescriptorAllocator
    {
    public:

        DescriptorAllocator(const VkDevice& device);
        ~DescriptorAllocator();

        bool AllocatePool(HashID layoutHash, const std::vector<DescriptorInfo>& descriptorInfos);
        bool AllocateSet(const Shader* const shader, DescriptorSet** outDescriptorSet);
    private:
        static constexpr uint16_t sMaxDescriptorCount = 64;
        static constexpr uint16_t sMaxDescriptorSetCount = 32;
    private:
        VkDevice mDevice;
        std::unordered_map<HashID, std::vector<VkDescriptorPool>> mDescriptorPools;
        std::unordered_map<HashID, uint16_t> mNowPoolIndices;
        // MEMO: 할당된 DescriptorSet 관리
        std::unordered_map<HashID, std::vector<DescriptorSet*>> mDescriptorSetPools;
    };

}
