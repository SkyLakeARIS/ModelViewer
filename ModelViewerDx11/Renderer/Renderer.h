#pragma once
#include "GfxPrerequisites.h"
#include "./Resources/RenderTypes.h"

namespace renderer
{
    class ResourceManager;
    class DescriptorAllocator;
    class FrameContext;
    class DescriptorSet;
    class Buffer;
    class Image;

    class Renderer final
    {
    private:
        // MEMO: Pool-Buffer = 1:1, 더블버퍼링
        static constexpr uint8_t sFrameInFlightCount = 2;
    public:

        // Texture
        struct TextureInfo
        {
            VkImage Image;
            VkDeviceMemory ImageMemory;
            VkImageView ImageView;
            uint32_t Layout;
            uint32_t Access;
        };
    public:
        Renderer();
        ~Renderer();

        bool initializeWithVulkan(HWND handleWindow, HINSTANCE handleInstance, int16_t width, int16_t height, const char* const appName);
        // TODO: 모델 띄운 후, 구조 개선 작업에서 전부 분리할 것들.
        void PrepareObjects();
        void UpdateScene(std::vector<std::pair<renderer::eCbType, void*>>& uniforms);
        void RenderScene();
        void DrawIndexed(uint32_t indexCount, uint32_t vertexOffset, uint32_t indexOffset);
        //
        void WaitForLastFrame() const;
        void BeginRender();
        void EndRender();

        void SubmitCommands() const;
        void Present() const;

        void CountFrame() const;

        void WaitForDevice() const;

        void UploadPushConstant(const PushConstantBlock& dataBlock, eShader shaderType) const;
        void BindDescriptorSet(const DescriptorSet* const descriptorSet);
        void BindPipeline(HashID hash) const;
        // pipeline
        bool TryCreateGraphicsPipeline(HashID& outPipelineHash, eShader shader, eVertexFormat vertexFormat, ePrimitiveTopology topology, eRasterType rasterType, eBlendState blendState, eDepthStencilState depthStencilState, eRenderPass renderPass);

        void AddVertexData(const void* const pData, uint32_t attribSize, uint32_t count, HashID hash, BufferRange& outRange);
        void AddIndexData(void* pData, uint32_t attribSize, uint32_t count, HashID hash, BufferRange& outRange);

        ResourceManager* GetResourceManager() const;
        DescriptorAllocator* GetDescriptorAllocator() const;
        VkDevice GetDevice() const;

    private:
        bool createInstance(const char* const appName);
        bool createSurface(HWND handleWindow, HINSTANCE handleInstance);
        bool choosePhysicalDevice();
        bool createLogicalDevice();
        bool createSwapChain(uint16_t width, uint16_t height);
        bool createImageViews();
        bool createSyncObjects();
        bool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flag, VkCommandPool& outCommandPool) const;
        bool createCommandBuffers(VkCommandPool commandPool, uint32_t bufferCount, VkCommandBufferLevel level, VkCommandBuffer& outBuffers) const;
        bool createDepthStencil();
    private:
        int16_t mWindowHeight;
        int16_t mWindowWidth;

        VkInstance mVkInstance;

        VkPhysicalDevice mPhysicalDevice;
        VkPhysicalDeviceProperties2 mPhysicalDeviceProperties;
        uint32_t mGraphicsQueueIndex;
        VkDevice mDevice;
        VkQueue mQueue;

        VkSurfaceKHR mSurface;
        VkSurfaceFormatKHR mSurfaceFormat;
        VkSwapchainKHR mSwapChain;
        VkExtent2D mSwapChainExtent;
        std::vector<VkImage> mImages;
        std::vector<VkImageView> mImageViews;
        Image* mDepthStencil;
        VkImageView mDepthStencilView;
        VkFormat mDepthStencilFormat;

        FrameContext* mFrameContext;
        std::vector<VkSemaphore> mRenderFinishedSemaphores;
        uint32_t mCurImageIndex;
        // pipeline
        ResourceManager* mResourceManager;
        // shader
        DescriptorAllocator* mDescriptorAllocator;
        // buffer
        Buffer* mVertexBuffer;
        Buffer* mIndexBuffer;
        //
        VkCommandBuffer mCommandBufferTransfer;
        VkCommandPool mCommandPoolTransfer;

    };
}
