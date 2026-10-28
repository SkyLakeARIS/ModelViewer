#include "Image.h"
#include "../VkHelper.h"

namespace renderer
{
    Image::Image(const VkDevice& device)
        : mDevice(device)
        , mImage(nullptr)
        , mDeviceMemory(nullptr)
    {
    }

    Image::~Image()
    {
        vkFreeMemory(mDevice, mDeviceMemory, nullptr);
        vkDestroyImage(mDevice, mImage, nullptr);
    }

    void Image::Create(VkImageLayout layout, uint32_t queueFamilyIndex, VkImageUsageFlags usage, VkExtent3D imageExtent, VkImageType type, VkFormat format, VkMemoryPropertyFlagBits memoryProperty)
    {
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.extent = imageExtent;
        imageCreateInfo.format = format;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.imageType = type;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = usage;
        imageCreateInfo.queueFamilyIndexCount = 1;
        imageCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = layout;
        vkCreateImage(mDevice, &imageCreateInfo, nullptr, &mImage);

        VkImageMemoryRequirementsInfo2 imageMemoryRequirements{};
        imageMemoryRequirements.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
        imageMemoryRequirements.image = mImage;

        VkMemoryRequirements2 memoryRequirements2{};
        memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        vkGetImageMemoryRequirements2(mDevice, &imageMemoryRequirements, &memoryRequirements2);

        uint32_t memoryType;
        VkHelper::GetMemoryType(memoryRequirements2.memoryRequirements, memoryProperty, memoryType);
        VkHelper::AllocateMemory(mDevice, memoryRequirements2.memoryRequirements.size, memoryType, mDeviceMemory);

        VkBindImageMemoryInfo bindImageMemoryInfo{};
        bindImageMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
        bindImageMemoryInfo.memory = mDeviceMemory;
        bindImageMemoryInfo.memoryOffset = 0;
        bindImageMemoryInfo.image = mImage;
        vkBindImageMemory2(mDevice, 1, &bindImageMemoryInfo);
    }

    void Image::Release()
    {
        // MEMO: Pooling 시에 활용할 리셋 함수
    }

    VkImage Image::GetImage() const
    {
        return mImage;
    }
}
