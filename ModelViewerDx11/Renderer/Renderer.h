#pragma once
#include "GfxPrerequisites.h"
#include "./Resources/RenderTypes.h"

namespace renderer
{
    class FrameContext;
    class ShaderModule;
    class Shader;
    class DescriptorSet;
    class Buffer;
    class Sampler;

    class Renderer final
    {
    private:
        // MEMO: Pool-Buffer = 1:1, 더블버퍼링
        static constexpr uint8_t sOverlappedFrame = 2;
    public:
        struct SubDataRange
        {
            uint32_t StartIndex;
            uint32_t Count;
        };
        struct BufferInfo
        {
            VkBuffer Buffer;
            VkDeviceMemory Memory;
            uint32_t Alignment;
            uint32_t AllocationSize;
            uint32_t Cursor;
            std::unordered_map<HashID, SubDataRange> SubResources;
        };

        struct ShaderStage
        {
            VkShaderModule ShaderModule;
            VkPipelineShaderStageCreateInfo PipelineShaderStage;
        };
        struct DescriptorInfo
        {
            VkDescriptorType Type;
            uint32_t Count;
        };
        struct ShaderLayout
        {
            VkDescriptorSetLayout SetLayout;
            VkPipelineLayout PipelineLayout;
            std::vector<DescriptorInfo> DescriptorInfos;
        };
        struct ShaderDescriptor
        {
            VkDescriptorSet Set[sOverlappedFrame];
            VkDescriptorPool Pool;
        };
        // pipeline
        struct PipelineStateBitField
        {
            uint64_t Value;
            uint64_t BitWidth;
        };
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

        // pipeline
        bool TryCreateGraphicsPipeline(HashID& outPipelineHash, eShader shader, eVertexFormat vertexFormat, ePrimitiveTopology topology, eRasterType rasterType, eBlendState blendState, eDepthStencilState depthStencilState, eRenderPass renderPass);

        void AddVertexData(const void* const pData, uint32_t attribSize, uint32_t count, HashID hash, BufferRange& outRange);
        void AddIndexData(void* pData, uint32_t attribSize, uint32_t count, HashID hash, BufferRange& outRange);

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
        bool createSamplers();
        bool createShaders();
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

        FrameContext* mFrameContext;
        std::vector<VkSemaphore> mRenderFinishedSemaphores;
        uint32_t mCurImageIndex;
        // pipeline
        VkPipeline mSimplePipeline;
        // shader
        std::unordered_map<HashID, ShaderModule*> mShaderModules;
        std::vector<Shader*> mShaders;
        // buffer
        Buffer* mVertexBuffer;
        Buffer* mIndexBuffer;
        // Texture
        std::unordered_map<HashID, TextureInfo> mTextures;
        std::unordered_map<eSamplerType, Sampler*> mSamplers;
        //
        VkCommandBuffer mCommandBufferTransfer;
        VkCommandPool mCommandPoolTransfer;

    };
}
