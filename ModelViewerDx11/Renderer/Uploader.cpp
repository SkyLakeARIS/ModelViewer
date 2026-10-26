#include <memory>
#include "Uploader.h"
#include "../Util/Macro.h"
#include "Resources/Buffer.h"

namespace renderer
{
    VkDevice Uploader::sDevice = {};
    VkCommandBuffer Uploader::sCommandBuffer = {};
    VkCommandPool Uploader::sCommandPool = {};
    VkQueue Uploader::sGraphicsQueue = {};
    VkFence Uploader::sFence = {};
    uint32_t Uploader::sGraphicsQueueFamilyIndex = {};

    bool Uploader::Initialize(VkDevice device, VkCommandBuffer buffer, VkCommandPool pool, VkQueue queue, uint32_t queueFamilyIndex)
    {
        sDevice = device;
        // TODO: Buffer/Pool은 여기에서 만들도록. Queue도 Transfer Queue로 지원할 경우 만들기.
        sCommandBuffer = buffer;
        sCommandPool = pool;
        sGraphicsQueue = queue;
        sGraphicsQueueFamilyIndex = queueFamilyIndex;

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        const VkResult result = vkCreateFence(sDevice, &fenceCreateInfo, nullptr, &sFence);

        if (result != VK_SUCCESS)
        {
            ASSERT(false, "Fence 생성 실패 resultCode(%d)", result);
            return false;
        }
        return true;
    }

    void Uploader::Release()
    {
        vkDestroyFence(sDevice, sFence, nullptr);
    }

    void Uploader::UploadBuffer(const void* const pData, uint32_t sizeInBytes, uint32_t offset, uint32_t memProperties, VkDeviceMemory dstMemory, VkBuffer dstBuffer)
    {
        if(memProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        {
            std::unique_ptr<Buffer> staging = std::make_unique<Buffer>(sDevice);
            staging->Create(sizeInBytes, Buffer::eBufferType::Staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sGraphicsQueueFamilyIndex);
            staging->AddDataOffset(pData, 1, sizeInBytes, 0);
            uploadBufferStaging(staging->GetBuffer(), dstBuffer, sizeInBytes, 0, offset);
        }
        else
        {
            uploadBufferMap(pData, sizeInBytes, offset, dstMemory);
        }
    }

    void Uploader::UploadImage(const void* const pData, uint32_t sizeInBytes, uint32_t offset, VkDeviceMemory memory, uint32_t memProperties)
    {
    }

    void Uploader::CopyBufferToBuffer(VkBuffer srcBuf, VkBuffer dstBuf, uint32_t copySize)
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(sCommandBuffer, &commandBufferBeginInfo);

        VkBufferCopy2 bufferCopy2{};
        bufferCopy2.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
        bufferCopy2.size = copySize;
        bufferCopy2.dstOffset = 0;
        bufferCopy2.srcOffset = 0;
        VkCopyBufferInfo2 copyBufferInfo2{};
        copyBufferInfo2.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
        copyBufferInfo2.srcBuffer = srcBuf;
        copyBufferInfo2.dstBuffer = dstBuf;
        copyBufferInfo2.pRegions = &bufferCopy2;
        copyBufferInfo2.regionCount = 1;

        vkCmdCopyBuffer2(sCommandBuffer, &copyBufferInfo2);

        vkEndCommandBuffer(sCommandBuffer);

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
        commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferSubmitInfo.commandBuffer = sCommandBuffer;
        commandBufferSubmitInfo.deviceMask = 1;

        VkSubmitInfo2 submitInfo2{};
        submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo2.commandBufferInfoCount = 1;
        submitInfo2.pCommandBufferInfos = &commandBufferSubmitInfo;
        vkQueueSubmit2(sGraphicsQueue, 1, &submitInfo2, sFence);

        const VkResult result = vkWaitForFences(sDevice, 1, &sFence, VK_FALSE, UINT64_MAX);
        ASSERT(result == VK_SUCCESS, "Uploader Fence 대기 실패");
    }

    void Uploader::CopyBufferToImage(VkBuffer srcBuf, VkImage dstImage, VkBufferImageCopy2& region)
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(sCommandBuffer, &commandBufferBeginInfo);

        TransitionImageLayout(sCommandBuffer, dstImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {}, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
        VkCopyBufferToImageInfo2 copyBufferToImageInfo2{};
        copyBufferToImageInfo2.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
        copyBufferToImageInfo2.srcBuffer = srcBuf;
        copyBufferToImageInfo2.dstImage = dstImage;
        copyBufferToImageInfo2.pRegions = &region;
        copyBufferToImageInfo2.regionCount = 1;

        vkCmdCopyBufferToImage2(sCommandBuffer, &copyBufferToImageInfo2);

        TransitionImageLayout(sCommandBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

        vkEndCommandBuffer(sCommandBuffer);

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
        commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferSubmitInfo.commandBuffer = sCommandBuffer;
        commandBufferSubmitInfo.deviceMask = 1;

        VkSubmitInfo2 submitInfo2{};
        submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo2.commandBufferInfoCount = 1;
        submitInfo2.pCommandBufferInfos = &commandBufferSubmitInfo;
        vkQueueSubmit2(sGraphicsQueue, 1, &submitInfo2, nullptr);

        vkQueueWaitIdle(sGraphicsQueue);
    }

    void Uploader::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask)
    {
        VkImageMemoryBarrier2 imageBarrier{};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarrier.image = image;

        imageBarrier.srcAccessMask = srcAccessMask;
        imageBarrier.dstAccessMask = dstAccessMask;

        imageBarrier.srcStageMask = srcStageMask;
        imageBarrier.dstStageMask = dstStageMask;

        imageBarrier.oldLayout = oldLayout;
        imageBarrier.newLayout = newLayout;

        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange = VkImageSubresourceRange{ aspectMask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };


        VkDependencyInfo dependencyInfo{};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.dependencyFlags = {};
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &imageBarrier;
        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
    }

    void Uploader::uploadBufferStaging(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t sizeInBytes, uint32_t srcOffset, uint32_t dstOffset)
    {
        VkCommandBufferBeginInfo bufferBeginInfo{};
        bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(sCommandBuffer, &bufferBeginInfo);

        VkBufferCopy2 bufferCopy2{};
        bufferCopy2.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
        bufferCopy2.dstOffset = dstOffset;
        bufferCopy2.size = sizeInBytes;
        bufferCopy2.srcOffset = srcOffset;
        VkCopyBufferInfo2 copyBufferInfo2{};
        copyBufferInfo2.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
        copyBufferInfo2.srcBuffer = srcBuffer;
        copyBufferInfo2.dstBuffer = dstBuffer;
        copyBufferInfo2.pRegions = &bufferCopy2;
        copyBufferInfo2.regionCount = 1;
        vkCmdCopyBuffer2(sCommandBuffer, &copyBufferInfo2);

        vkEndCommandBuffer(sCommandBuffer);

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
        commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferSubmitInfo.commandBuffer = sCommandBuffer;
        commandBufferSubmitInfo.deviceMask = 1;

        VkSubmitInfo2 submitInfo2{};
        submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo2.commandBufferInfoCount = 1;
        submitInfo2.pCommandBufferInfos = &commandBufferSubmitInfo;
        vkQueueSubmit2(sGraphicsQueue, 1, &submitInfo2, nullptr);

        vkQueueWaitIdle(sGraphicsQueue);
    }

    void Uploader::uploadBufferMap(const void* const pData, uint32_t sizeInBytes, uint32_t offset, VkDeviceMemory memory)
    {
        VkMemoryMapInfo memoryMapInfo{};
        memoryMapInfo.sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO;
        memoryMapInfo.memory = memory;
        memoryMapInfo.size = sizeInBytes;
        memoryMapInfo.offset = offset;
        memoryMapInfo.flags = 0;

        void* pBufferCursor = nullptr;
        vkMapMemory2(sDevice, &memoryMapInfo, &pBufferCursor);

        (void)memcpy(pBufferCursor, pData, sizeInBytes);

        VkMemoryUnmapInfo memoryUnmapInfo{};
        memoryUnmapInfo.sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO;
        memoryUnmapInfo.memory = memory;
        vkUnmapMemory2(sDevice, &memoryUnmapInfo);
    }
}
