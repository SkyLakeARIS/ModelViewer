#include "VkHelper.h"
#include "../Util/Macro.h"

namespace renderer
{
    uint32_t VkHelper::sGraphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t VkHelper::sTransferQueueFamilyIndex = UINT32_MAX;
    VkPhysicalDeviceMemoryProperties VkHelper::sDeviceMemoryProperties = {};
    bool VkHelper::sbSupportStagingUpload = false;

    bool VkHelper::AllocateMemory(const VkDevice& device, uint32_t allocSize, uint32_t memoryType, VkDeviceMemory& outMemory)
    {
        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = allocSize;
        memoryAllocateInfo.memoryTypeIndex = memoryType;

        const VkResult result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &outMemory);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "AllocateMemory 실패. allocSize(%u), memoryType(%u) resultCode(%d)", allocSize, memoryType, result);
            return false;
        }
        return true;
    }

    void VkHelper::GetMemoryType(const VkMemoryRequirements& memoryRequirements, uint32_t flag, uint32_t& outMemoryType)
    {
        ASSERT(sDeviceMemoryProperties.memoryTypeCount != 0 && sDeviceMemoryProperties.memoryHeapCount != 0, "초기화 시 sDeviceMemoryProperties가 등록되지 않은 것 같습니다.");

        for (uint32_t bit = 0; bit < sDeviceMemoryProperties.memoryTypeCount; ++bit)
        {
            if (memoryRequirements.memoryTypeBits & (1 << bit) &&
                sDeviceMemoryProperties.memoryTypes[bit].propertyFlags == flag)
            {
                outMemoryType = bit;
            }
        }
    }

    uint32_t VkHelper::AlignedSize(uint32_t value, uint32_t alignment)
    {
        return ((((value) + (alignment) - 1) / (alignment)) * (alignment));
    }

    void VkHelper::SetQueueFamilyIndices(uint32_t graphics, uint32_t transfer)
    {
        sGraphicsQueueFamilyIndex = graphics;
        sTransferQueueFamilyIndex = transfer;
    }

    void VkHelper::SetPhysicalDeviceMemoryProperties(const VkPhysicalDeviceMemoryProperties& properties)
    {
        sDeviceMemoryProperties = properties;
    }

    void VkHelper::SetSupportStagingUpload(bool bEnable)
    {
        sbSupportStagingUpload = bEnable;
    }

    uint32_t VkHelper::GetGraphicsQueueFamilyIndex()
    {
        return sGraphicsQueueFamilyIndex;
    }

    uint32_t VkHelper::GetTransferQueueFamilyIndex()
    {
        return sTransferQueueFamilyIndex;
    }

    bool VkHelper::IsSupportStagingUpload()
    {
        return sbSupportStagingUpload;
    }
}
