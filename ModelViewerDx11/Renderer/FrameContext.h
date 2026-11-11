#pragma once
#include "GfxPrerequisites.h"
#include "../Util/Type.h"
#include "Shader/ShaderType.h"

namespace renderer
{
    class Sampler;
    class Shader;
    class DescriptorSet;
    class Image;

    struct FrameData
    {
        VkCommandPool Pool;
        VkCommandBuffer Buffer;
        VkSemaphore ImageReadySemaphore;
        VkFence RenderFinishedFence;
    };

    class FrameContext
    {
    public:
        FrameContext(const VkDevice& device);
        ~FrameContext();

        bool Initialize(DescriptorSet* const commonDescriptorSet, Sampler* const shadowMapSampler);

        uint32_t AcquireSwapchainImage(const VkSwapchainKHR& swapchain) const;
        void WaitForLastFrame() const;
        void ResetFence() const;

        void CountFrame();

        void UpdateUniformData(const void* const pData, uint32_t count, uint32_t elementSize) const;

        uint32_t GetNowInFlightIndex() const;
        VkSemaphore GetImageReadySemaphoreInFlight() const;
        VkFence GetRenderFinishedSemaphoreInFlight() const;
        VkCommandBuffer GetCommandBufferInFlight() const;
        VkDescriptorSet GetCommandDescriptorSetInFlight() const;
        VkImage GetShadowImageInFlight() const;
        VkImageView GetShadowImageViewInFlight() const;
        VkExtent2D GetShadowMapResolution() const;
        VkFormat GetShadowImageFormat() const;
    private:
        bool createShadowMapResources();
    private:
        static const uint32_t sShadowMapResolution;
    private:
        VkDevice mDevice;
        uint32_t mInFlightCount;
        std::vector<FrameData> mInFlights;
        uint32_t mNowInFlightIndex;
        DescriptorSet* mCommonDescriptorSet;
        std::vector<Image*> mShadowImages;
        std::vector<VkImageView> mShadowImageViews;
        VkFormat mShadowImageFormat;
    };
}
