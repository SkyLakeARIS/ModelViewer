#include "DescriptorAllocator.h"
#include "DescriptorSet.h"
#include "../VkHelper.h"
#include "../../Util/Macro.h"
#include "../Shader/Shader.h"

namespace renderer
{
    DescriptorAllocator::DescriptorAllocator(const VkDevice& device)
        : mDevice(device)
    {
    }

    DescriptorAllocator::~DescriptorAllocator()
    {
        for (auto& setPoolIt : mDescriptorSetPools)
        {
            for (const auto& set : setPoolIt.second)
            {
                delete set;
            }
            setPoolIt.second.clear();
            std::vector<DescriptorSet*>().swap(setPoolIt.second);
        }
        mDescriptorSetPools.clear();

        for(auto& poolsIt : mDescriptorPools)
        {
            for(const auto& pool : poolsIt.second)
            {
                vkDestroyDescriptorPool(mDevice, pool, nullptr);
            }
            poolsIt.second.clear();
            std::vector<VkDescriptorPool>().swap(poolsIt.second);
        }
        mDescriptorPools.clear();
    }

    bool DescriptorAllocator::AllocatePool(HashID layoutHash, const std::vector<DescriptorInfo>& descriptorInfos)
    {
        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.reserve(descriptorInfos.size());
        for (const auto& descriptorInfo : descriptorInfos)
        {
            const VkDescriptorPoolSize poolSize{ descriptorInfo.Type, descriptorInfo.Count * VkHelper::GetFrameInFlightCount() * sMaxDescriptorSetCount };
            poolSizes.push_back(poolSize);
        }

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptorPoolCreateInfo.maxSets = sMaxDescriptorSetCount;
        descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
        descriptorPoolCreateInfo.poolSizeCount = poolSizes.size();
        VkDescriptorPool pool;
        const VkResult result = vkCreateDescriptorPool(mDevice, &descriptorPoolCreateInfo, nullptr, &pool);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "DescriptorAllocator 생성 실패. layoutHash(%u), poolSizeCount(%d)", layoutHash, poolSizes.size());
            return false;
        }

        const auto& poolIt = mDescriptorPools.find(layoutHash);
        if (poolIt == mDescriptorPools.end())
        {
            std::vector<VkDescriptorPool> pools(1, pool);
            mDescriptorPools.insert({ layoutHash, std::move(pools) });
        }
        else
        {
            poolIt->second.push_back(pool);
        }

        const auto& poolIndexIt = mNowPoolIndices.find(layoutHash);
        if(poolIndexIt == mNowPoolIndices.end())
        {
            mNowPoolIndices.insert({ layoutHash , 0});
        }

        return true;
    }

    bool DescriptorAllocator::AllocateSet(const Shader* const shader, DescriptorSet** outDescriptorSet)
    {
        ASSERT(*outDescriptorSet == nullptr, "outDescriptorSet는 nullptr여야합니다.");

        const HashID layoutHash = shader->GetLayoutHash();
        auto poolIndexIt = mNowPoolIndices.find(layoutHash);
        if(poolIndexIt == mNowPoolIndices.end())
        {
            ASSERT(false, "먼저 Pool을 할당해야 합니다. layoutHash(%u)", layoutHash);
            return false;
        }
        const auto& poolIt = mDescriptorPools.find(layoutHash);
        uint16_t nowPoolIndex = poolIndexIt->second;
        const uint32_t inFlightCount = VkHelper::GetFrameInFlightCount();
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts(inFlightCount, shader->GetDescriptorSetLayout());
        VkDescriptorSetAllocateInfo setAllocateInfo{};
        setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setAllocateInfo.descriptorPool = poolIt->second[nowPoolIndex];
        setAllocateInfo.descriptorSetCount = descriptorSetLayouts.size();
        setAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

        std::vector<VkDescriptorSet> descriptorSets(inFlightCount);
        VkResult result = vkAllocateDescriptorSets(mDevice, &setAllocateInfo, descriptorSets.data());
        if(result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
        {
            if (AllocatePool(layoutHash, shader->GetDescriptorInfos()) == false)
            {
                ASSERT(false, "Pool을 확장하지 못했습니다. layoutHash(%u)", layoutHash);
                return false;
            }

            ++poolIndexIt->second;
            nowPoolIndex = poolIndexIt->second;

            setAllocateInfo.descriptorPool = poolIt->second[nowPoolIndex];
            result = vkAllocateDescriptorSets(mDevice, &setAllocateInfo, descriptorSets.data());
        }

        if (result != VK_SUCCESS)
        {
            ASSERT(false, "DescriptorSet 생성 실패.  result(%u)", result);
            return false;
        }

        const PoolInfo ownerInfo{ nowPoolIndex, layoutHash };
        *outDescriptorSet = new DescriptorSet(mDevice, descriptorSets, ownerInfo);
        if ((*outDescriptorSet)->Create() == false)
        {
            delete outDescriptorSet;
            outDescriptorSet = nullptr;
            ASSERT(false, "DescriptorSet 개체 초기화 실패.  result(%u)", result);
            return false;
        }

        const auto descriptorSetPoolIt = mDescriptorSetPools.find(layoutHash);
        if(descriptorSetPoolIt == mDescriptorSetPools.end())
        {
            std::vector<DescriptorSet*> descriptorSetPools;
            descriptorSetPools.reserve(4);
            descriptorSetPools.push_back(*outDescriptorSet);
            mDescriptorSetPools.insert({ layoutHash, std::move(descriptorSetPools) });
        }
        else
        {
            descriptorSetPoolIt->second.push_back(*outDescriptorSet);
        }
        return true;
    }
}
