#include "FrameContext.h"
#include "../Util/Macro.h"
#include "Shader/DescriptorSet.h"
#include "VkHelper.h"
#include "Shader/DescriptorLayout.h"
#include "Shader/Shader.h"

namespace renderer
{
    FrameContext::FrameContext(const VkDevice& device)
        : mDevice(device)
        , mInFlightCount(0)
        , mNowInFlightIndex(0)
        , mCommonDescriptorSet(nullptr)
    {
    }

    FrameContext::~FrameContext()
    {
        mCommonDescriptorSet = nullptr;

        for (const auto& frame : mInFlights)
        {
            vkDestroyCommandPool(mDevice, frame.Pool, nullptr);
            vkDestroySemaphore(mDevice, frame.ImageReadySemaphore, nullptr);
            vkDestroyFence(mDevice, frame.RenderFinishedFence, nullptr);
        }
        mInFlights.clear();
        std::vector<FrameData>().swap(mInFlights);
    }

    bool FrameContext::Initialize(uint32_t inFlightCount, const std::vector<Shader*>& shaders)
    {
        mInFlightCount = inFlightCount;

        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.queueFamilyIndex = VkHelper::GetGraphicsQueueFamilyIndex();
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandBufferCount = 1;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        mInFlights.resize(inFlightCount);
        for (auto& frame : mInFlights)
        {
            VkResult result = vkCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &frame.Pool);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "CommandPool 생성 실패 resultCode(%d)", result);
                return false;
            }

            commandBufferAllocateInfo.commandPool = frame.Pool;
            result = vkAllocateCommandBuffers(mDevice, &commandBufferAllocateInfo, &frame.Buffer);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "CommandBuffer 생성 실패 resultCode(%d)", result);
                return false;
            }

            result = vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &frame.ImageReadySemaphore);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "Semaphore(ImageReady) 생성 실패 resultCode(%d)", result);
                return false;
            }

            result = vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &frame.RenderFinishedFence);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "Fence 생성 실패 resultCode(%d)", result);
                return false;
            }
        }
        return true;
    }

    bool FrameContext::Initialize(DescriptorSet* const commonDescriptorSet)
    {
        mInFlightCount = VkHelper::GetFrameInFlightCount();

        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.queueFamilyIndex = VkHelper::GetGraphicsQueueFamilyIndex();
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandBufferCount = 1;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        mInFlights.resize(mInFlightCount);
        for (auto& frame : mInFlights)
        {
            VkResult result = vkCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &frame.Pool);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "CommandPool 생성 실패 resultCode(%d)", result);
                return false;
            }

            commandBufferAllocateInfo.commandPool = frame.Pool;
            result = vkAllocateCommandBuffers(mDevice, &commandBufferAllocateInfo, &frame.Buffer);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "CommandBuffer 생성 실패 resultCode(%d)", result);
                return false;
            }

            result = vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &frame.ImageReadySemaphore);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "Semaphore(ImageReady) 생성 실패 resultCode(%d)", result);
                return false;
            }

            result = vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &frame.RenderFinishedFence);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "Fence 생성 실패 resultCode(%d)", result);
                return false;
            }
        }
        const eLayoutType layoutType = commonDescriptorSet->GetLayoutType();
        std::vector<DescriptorWriteInfo> writeInfos;
        DescriptorLayout::GetLayoutWriteInfos(layoutType, writeInfos);
        commonDescriptorSet->UpdateDescriptorSet(writeInfos);
        mCommonDescriptorSet = commonDescriptorSet;
        return true;
    }

    uint32_t FrameContext::AcquireSwapchainImage(const VkSwapchainKHR& swapchain) const
    {
        uint32_t nextImageIndex = UINT32_MAX;
        VkAcquireNextImageInfoKHR acqNextImageInfo{};
        acqNextImageInfo.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
        acqNextImageInfo.semaphore = mInFlights[mNowInFlightIndex].ImageReadySemaphore;
        acqNextImageInfo.swapchain = swapchain;
        acqNextImageInfo.timeout = 1'000'000'000; // 1sec
        acqNextImageInfo.deviceMask = 1;

        const VkResult result = vkAcquireNextImage2KHR(mDevice, &acqNextImageInfo, &nextImageIndex);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "next image 획득 실패 resultCode(%d)", result);
        }
        //ASSERT(nextImageIndex < mImages.size(), "유효하지 않은 image Index를 획득했거나, index 획득에 실패했습니다.  nextImageIndex(%u)", nextImageIndex);
        return nextImageIndex;
    }

    void FrameContext::WaitForLastFrame() const
    {
        // wait 1sec = 1'000'000'000
        const VkResult result = vkWaitForFences(mDevice, 1, &mInFlights[mNowInFlightIndex].RenderFinishedFence, VK_TRUE, 1'000'000'000);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "Fence Wait LastFrame 실패 resultCode(%d)", result);
        }
    }

    void FrameContext::ResetFence() const
    {
        (void)vkResetFences(mDevice, 1, &mInFlights[mNowInFlightIndex].RenderFinishedFence);
    }

    void FrameContext::CountFrame()
    {
        mNowInFlightIndex = (mNowInFlightIndex + 1) % mInFlightCount;
    }

    void FrameContext::UpdateUniformData(eShader shaderType, const void* const pData, uint32_t count, uint32_t elementSize, bool bPerFrame) const
    {
        return mCommonDescriptorSet->UpdateUniformData(pData, count, elementSize, mNowInFlightIndex, bPerFrame);
    }

    VkSemaphore FrameContext::GetImageReadySemaphoreInFlight() const
    {
        return mInFlights[mNowInFlightIndex].ImageReadySemaphore;
    }

    VkFence FrameContext::GetRenderFinishedSemaphoreInFlight() const
    {
        return mInFlights[mNowInFlightIndex].RenderFinishedFence;
    }

    VkCommandBuffer FrameContext::GetCommandBufferInFlight() const
    {
        return mInFlights[mNowInFlightIndex].Buffer;
    }

    VkDescriptorSet FrameContext::GetCommandDescriptorSetInFlight() const
    {
        return mCommonDescriptorSet->GetDescriptorSet(mNowInFlightIndex);
    }
}
