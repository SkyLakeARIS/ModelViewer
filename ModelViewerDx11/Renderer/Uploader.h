#pragma once
#include "GfxPrerequisites.h"

namespace renderer
{
    class Uploader final
    {
    public:
        static bool Initialize(VkDevice device, VkCommandBuffer buffer, VkCommandPool pool, VkQueue queue, uint32_t queueFamilyIndex);
        static void Release();

        // memProperties에 따라서 변경.
        static void UploadBuffer(const void* const pData, uint32_t sizeInBytes, uint32_t offset, uint32_t memProperties, VkDeviceMemory dstMemory, VkBuffer dstBuffer);
        static void UploadImage(const void* const pData, uint32_t sizeInBytes, uint32_t offset, VkDeviceMemory memory, uint32_t memProperties);

        static void CopyBufferToBuffer(VkBuffer srcBuf, VkBuffer dstBuf, uint32_t copySize);
        static void CopyBufferToImage(VkBuffer srcBuf, VkImage dstImage, VkBufferImageCopy2& region);

        static void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask, VkImageAspectFlags aspectMask);

    private:
        static void uploadBufferStaging(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t sizeInBytes, uint32_t srcOffset, uint32_t dstOffset);
        static void uploadBufferMap(const void* const pData, uint32_t sizeInBytes, uint32_t offset, VkDeviceMemory memory);
        
    private:
        Uploader() = default;
        ~Uploader() = default;
    private:
        static VkDevice sDevice;
        static VkCommandBuffer sCommandBuffer;
        static VkCommandPool sCommandPool;
        static VkQueue sGraphicsQueue;
        static VkFence sFence;
        static uint32_t sGraphicsQueueFamilyIndex;
    };
}
