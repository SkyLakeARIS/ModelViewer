#pragma once
#include "../GfxPrerequisites.h"

namespace renderer
{
    class Image
    {
    public:
        Image(const VkDevice& device);
        ~Image();

        void Create(VkImageLayout layout, uint32_t queueFamilyIndex, VkImageUsageFlags usage, VkExtent3D imageExtent, uint32_t imageCount, VkImageType type, VkFormat format, VkImageCreateFlags createFlags, VkMemoryPropertyFlagBits memoryProperty);

        void Release();

        VkImage GetImage() const;
    private:
        VkDevice mDevice;
        VkImage mImage;
        VkDeviceMemory mDeviceMemory;
        uint32_t mImageCount;
    };
}
