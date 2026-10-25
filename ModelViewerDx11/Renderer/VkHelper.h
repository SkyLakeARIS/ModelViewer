#pragma once
#include "GfxPrerequisites.h"

namespace renderer
{
    class VkHelper final
    {
    public:
        static bool AllocateMemory(const VkDevice& device, uint32_t allocSize, uint32_t memoryType, VkDeviceMemory& outMemory);
        static void GetMemoryType(const VkMemoryRequirements& memoryRequirements, uint32_t flag, uint32_t& outMemoryType);

        static uint32_t AlignedSize(uint32_t value, uint32_t alignment);

        static void SetQueueFamilyIndices(uint32_t graphics, uint32_t transfer);
        static void SetPhysicalDeviceMemoryProperties(const VkPhysicalDeviceMemoryProperties& properties);
        static void SetSupportStagingUpload(bool bEnable);

        static uint32_t GetGraphicsQueueFamilyIndex();
        static uint32_t GetTransferQueueFamilyIndex();

        static bool IsSupportStagingUpload();
    private:
        VkHelper() = default;
        ~VkHelper() = default;
    private:
        static uint32_t sGraphicsQueueFamilyIndex;
        static uint32_t sTransferQueueFamilyIndex;
        static VkPhysicalDeviceMemoryProperties sDeviceMemoryProperties;
        static bool sbSupportStagingUpload;
    };
}
