#pragma once
#include "GfxPrerequisites.h"
#include "../Util/Type.h"
#include "Shader/ShaderType.h"

namespace renderer
{
    class Shader;
    class DescriptorSet;

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

        bool Initialize(uint32_t inFlightCount, const std::vector<Shader*>& shaders);
        bool Initialize(DescriptorSet* const commonDescriptorSet);

        uint32_t AcquireSwapchainImage(const VkSwapchainKHR& swapchain) const;
        void WaitForLastFrame() const;
        void ResetFence() const;

        void CountFrame();

        void UpdateUniformData(eShader shaderType, const void* const pData, uint32_t count, uint32_t elementSize, bool bPerFrame) const;

        uint32_t GetNowInFlightIndex() const;
        VkSemaphore GetImageReadySemaphoreInFlight() const;
        VkFence GetRenderFinishedSemaphoreInFlight() const;
        VkCommandBuffer GetCommandBufferInFlight() const;
        VkDescriptorSet GetCommandDescriptorSetInFlight() const;
    private:
        VkDevice mDevice;
        uint32_t mInFlightCount;
        std::vector<FrameData> mInFlights;
        uint32_t mNowInFlightIndex;
        DescriptorSet* mCommonDescriptorSet;
    };
}
