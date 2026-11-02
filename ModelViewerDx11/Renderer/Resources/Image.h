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

        VkImage GetImage() const;
    private:
        VkDevice mDevice;
        VkImage mImage;
        VkDeviceMemory mDeviceMemory;
    };
}
