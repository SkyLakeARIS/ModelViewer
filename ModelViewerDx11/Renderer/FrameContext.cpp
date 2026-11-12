#include "FrameContext.h"
#include "../Util/Macro.h"
#include "Shader/DescriptorSet.h"
#include "VkHelper.h"
#include "Resources/Image.h"
#include "Shader/DescriptorLayout.h"

namespace renderer
{
    const uint32_t FrameContext::sShadowMapResolution = 2048;

    FrameContext::FrameContext(const VkDevice& device)
        : mDevice(device)
        , mInFlightCount(0)
        , mNowInFlightIndex(0)
        , mCommonDescriptorSet(nullptr)
        , mShadowImageFormat()
    {
    }

    FrameContext::~FrameContext()
    {
        mCommonDescriptorSet = nullptr;

        for(auto& shadowImage : mShadowImages)
        {
            delete shadowImage;
            shadowImage = nullptr;
        }
        mShadowImages.clear();
        std::vector<Image*>().swap(mShadowImages);

        for (auto& shadowImageView : mShadowImageViews)
        {
            vkDestroyImageView(mDevice, shadowImageView, nullptr);
        }
        mShadowImageViews.clear();
        std::vector<VkImageView>().swap(mShadowImageViews);

        for (const auto& frame : mInFlights)
        {
            vkDestroyCommandPool(mDevice, frame.Pool, nullptr);
            vkDestroySemaphore(mDevice, frame.ImageReadySemaphore, nullptr);
            vkDestroyFence(mDevice, frame.RenderFinishedFence, nullptr);
        }
        mInFlights.clear();
        std::vector<FrameData>().swap(mInFlights);
    }

    bool FrameContext::Initialize(DescriptorSet* const commonDescriptorSet, Sampler* const shadowMapSampler)
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

        if(createShadowMapResources() == false)
        {
            ASSERT(false, "셰도우 이미지 리소스 생성 실패");
            return false;
        }

        std::vector<DescriptorWriteInfo> writeInfos;
        DescriptorLayout::GetLayoutWriteInfos(eLayoutType::Common, writeInfos);
        for (uint32_t inFlightIndex = 0; inFlightIndex < mInFlightCount; ++inFlightIndex)
        {
            for (auto& writeInfo : writeInfos)
            {
                if (writeInfo.bTexture)
                {
                    writeInfo.ImageView = mShadowImageViews[inFlightIndex];
                }
                else if (writeInfo.Descriptor.Type == VK_DESCRIPTOR_TYPE_SAMPLER)
                {
                    writeInfo.Sampler = shadowMapSampler;
                }
            }
            commonDescriptorSet->UpdateDescriptorSet(writeInfos, inFlightIndex);
        }
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

    void FrameContext::UpdateUniformData(const void* const pData, uint32_t count, uint32_t elementSize) const
    {
        return mCommonDescriptorSet->UpdateUniformData(pData, count, elementSize, mNowInFlightIndex);
    }

    uint32_t FrameContext::GetNowInFlightIndex() const
    {
        return mNowInFlightIndex;
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

    VkImage FrameContext::GetShadowImageInFlight() const
    {
        return mShadowImages[mNowInFlightIndex]->GetImage();
    }

    VkImageView FrameContext::GetShadowImageViewInFlight() const
    {
        return mShadowImageViews[mNowInFlightIndex];
    }

    VkExtent2D FrameContext::GetShadowMapResolution() const
    {
        return { sShadowMapResolution, sShadowMapResolution };
    }

    VkFormat FrameContext::GetShadowImageFormat() const
    {
        return mShadowImageFormat;
    }

    bool FrameContext::createShadowMapResources()
    {
        mShadowImages.reserve(mInFlightCount);
        mShadowImageViews.reserve(mInFlightCount);
        const uint32_t graphicsQueueFamilyIndex = VkHelper::GetGraphicsQueueFamilyIndex();
        mShadowImageFormat = VkHelper::GetSupportImageFormat({ VK_FORMAT_D32_SFLOAT , VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        constexpr VkExtent3D SHADOW_IMAGE_EXTENT{ sShadowMapResolution, sShadowMapResolution, 1 };
        for (uint32_t frame = 0; frame < mInFlightCount; ++frame)
        {
            Image* shadowImage = new Image(mDevice);

            shadowImage->Create({}, graphicsQueueFamilyIndex, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, SHADOW_IMAGE_EXTENT, 1, VK_IMAGE_TYPE_2D, mShadowImageFormat, {}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            mShadowImages.push_back(shadowImage);

            VkImageViewCreateInfo imageCreateInfo{};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageCreateInfo.format = mShadowImageFormat;
            imageCreateInfo.components = {};
            imageCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
            imageCreateInfo.image = shadowImage->GetImage();

            VkImageView shadowImageView;
            const VkResult result = vkCreateImageView(mDevice, &imageCreateInfo, nullptr, &shadowImageView);
            if(result != VK_SUCCESS)
            {
                ASSERT(false, "ShadowImageView 생성 실패 resultCode(%d)", result);
                return false;
            }
            mShadowImageViews.push_back(shadowImageView);
        }
        return true;
    }
}
