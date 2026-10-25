#pragma once
#include "GfxPrerequisites.h"

namespace renderer
{
    class Uploader final
    {
    public:
        static void Initialize(VkDevice device, VkCommandBuffer buffer, VkCommandPool pool, VkQueue queue, uint32_t queueFamilyIndex);
        static void Release();

        // memProperties에 따라서 변경.
        static void UploadBuffer(const void* const pData, uint32_t sizeInBytes, uint32_t offset, uint32_t memProperties, VkDeviceMemory dstMemory, VkBuffer dstBuffer);
        // TODO: 업로드 할 때 필요한 인자만 재정리하면 될 듯, 방식은 staging 방식으로 고정
        static void UploadImage(const void* const pData, uint32_t sizeInBytes, uint32_t offset, VkDeviceMemory memory, uint32_t memProperties);

        // TODO: 순수 버퍼간 복사 지원
        static void CopyBufferToBuffer(VkBuffer srcBuf, VkBuffer dstBuf, uint32_t copySize);
        static void CopyBufferToImage(VkBuffer srcBuf, VkImage dstImage, VkBufferImageCopy2& region);

        static void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask);

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
