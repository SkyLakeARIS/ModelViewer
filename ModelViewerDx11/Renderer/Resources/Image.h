#pragma once
#include "../GfxPrerequisites.h"

namespace renderer
{
    class Image
    {
    public:
        Image(const VkDevice& device);
        ~Image();

        void Create(VkImageLayout layout, uint32_t queueFamilyIndex, VkImageUsageFlags usage, VkExtent3D imageExtent, VkImageType type, VkFormat format, VkMemoryPropertyFlagBits memoryProperty);
        void Release();
        // MEMO: transition을 어디서 해야 할까? commandBuffer 가 필요한데. ->현재 보통 staging을 위한 것이므로 Uploader에서.

        VkImage GetImage() const;
    private:
        VkDevice mDevice;
        VkImage mImage;
        VkDeviceMemory mDeviceMemory;
    };
}
