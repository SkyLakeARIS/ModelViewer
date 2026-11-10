#include "Renderer.h"
#include <algorithm>
#include "Shader/DescriptorSet.h"
#include "FrameContext.h"
#include "Uploader.h"
#include "VkHelper.h"
#include "../Util/Macro.h"
#include "../Util/Util.h"
#include "Resources/Buffer.h"
#include "Resources/Image.h"
#include "Resources/PipelineBuilder.h"
#include "Resources/ResourceManager.h"
#include "Shader/DescriptorAllocator.h"
#include "Shader/DescriptorLayout.h"

namespace renderer
{
    Renderer::Renderer()
        : mWindowHeight(0)
        , mWindowWidth(0)
        , mVkInstance(nullptr)
        , mPhysicalDevice()
        , mPhysicalDeviceProperties()
        , mGraphicsQueueIndex(UINT_MAX)
        , mDevice(nullptr)
        , mQueue(nullptr)
        , mSurface(nullptr)
        , mSurfaceFormat()
        , mSwapChain(nullptr)
        , mSwapChainExtent()
        , mDepthStencil(nullptr)
        , mDepthStencilView(nullptr)
        , mDepthStencilFormat()
        , mFrameContext(nullptr)
        , mCurImageIndex(0)
        , mResourceManager(nullptr)
        , mDescriptorAllocator(nullptr)
        , mCommandBufferTransfer(nullptr)
        , mCommandPoolTransfer(nullptr)
    {
    }

    Renderer::~Renderer()
    {
        Uploader::Release();

        delete mResourceManager;


        DescriptorLayout::Release();


        delete mFrameContext;
        mFrameContext = nullptr;
        vkDestroyCommandPool(mDevice, mCommandPoolTransfer, nullptr);

        delete mDescriptorAllocator;

    	for(const auto& semaphore : mRenderFinishedSemaphores)
        {
            vkDestroySemaphore(mDevice, semaphore, nullptr);
        }

        mImages.clear();
        std::vector<VkImage>().swap(mImages);
        for (auto& imageView : mImageViews)
        {
            vkDestroyImageView(mDevice, imageView, nullptr);
        }
        mImageViews.clear();
        std::vector<VkImageView>().swap(mImageViews);

        vkDestroyImageView(mDevice, mDepthStencilView, nullptr);
        delete mDepthStencil;
        mDepthStencil = nullptr;

        vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
        vkDestroySurfaceKHR(mVkInstance, mSurface, nullptr);
        vkDestroyDevice(mDevice, nullptr);
        vkDestroyInstance(mVkInstance, nullptr);
    }

    bool Renderer::initializeWithVulkan(HWND handleWindow, HINSTANCE handleInstance, int16_t width, int16_t height, const char* const appName)
    {
        mWindowWidth = width;
        mWindowHeight = height;
        VkHelper::SetFrameInFlightCount(sFrameInFlightCount);
        if(createInstance(appName) == false)
        {
            return false;
        }

        if(createSurface(handleWindow, handleInstance) == false)
        {
            return false;
        }

        if(choosePhysicalDevice() == false)
        {
            return false;
        }
        VkHelper::SetPhysicalDevice(mPhysicalDevice);

        VkPhysicalDeviceMemoryProperties2 physicalDeviceMemoryProperties2{};
        physicalDeviceMemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

        vkGetPhysicalDeviceMemoryProperties2(mPhysicalDevice, &physicalDeviceMemoryProperties2);
        VkHelper::SetPhysicalDeviceMemoryProperties(physicalDeviceMemoryProperties2.memoryProperties);


        if(createLogicalDevice() == false)
        {
            return false;
        }
        mResourceManager = new ResourceManager(mDevice);
        VkHelper::SetQueueFamilyIndices(mGraphicsQueueIndex, UINT32_MAX);

        if(createSwapChain(width, height) == false)
        {
            return false;
        }

        if (createImageViews() == false)
        {
            return false;
        }

        if (createDepthStencil() == false)
        {
            return false;
        }

        if(createSyncObjects() == false)
        {
            return false;
        }

        DescriptorLayout::Initialize(mDevice);
        if(mResourceManager->CreateShaders() == false)
        {
            return false;
        }

        mDescriptorAllocator = new DescriptorAllocator(mDevice);
        for (EnumInt layoutType = 0; layoutType < static_cast<EnumInt>(eLayoutType::LayoutCount); ++layoutType)
        {
            if (mDescriptorAllocator->AllocatePool(static_cast<eLayoutType>(layoutType)) == false)
            {
                return false;
            }
        }

        DescriptorSet* commonDescriptorSet = nullptr;
        if(mDescriptorAllocator->AllocateSet(eLayoutType::Common, &commonDescriptorSet) == false)
        {
            return false;
        }
        mFrameContext = new FrameContext(mDevice);
        if(mFrameContext->Initialize(commonDescriptorSet) == false)
        {
            return false;
        }

        if (mResourceManager->CreateSamplers() == false)
        {
            return false;
        }

        if(createCommandPool(mGraphicsQueueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, mCommandPoolTransfer) == false)
        {
            return false;
        }
        
        if(createCommandBuffers(mCommandPoolTransfer, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, mCommandBufferTransfer) == false)
        {
            return false;
        }

        if(Uploader::Initialize(mDevice, mCommandBufferTransfer, mCommandPoolTransfer, mQueue, mGraphicsQueueIndex) == false)
        {
            return false;
        }

        if(mResourceManager->CreateBuffers() == false)
        {
            return false;
        }

        return true;
    }

    void Renderer::UpdateScene(const PerFrameUniformBlock& uniformBlock)
    {
        mFrameContext->UpdateUniformData(&uniformBlock, 1, sizeof(PerFrameUniformBlock));
    }

    void Renderer::RenderScene()
    {
        // Scene Update
        const VkCommandBuffer cmdBuffer = mFrameContext->GetCommandBufferInFlight();
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = mSwapChainExtent.width;
        viewport.height = mSwapChainExtent.height;
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        VkRect2D scissor{ {0, 0}, mSwapChainExtent };
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);



        const VkDeviceSize offset = 0;
        const Buffer* vertexBuffer = mResourceManager->GetBuffer(true);
        const VkBuffer bufferVk = vertexBuffer->GetBuffer();
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &bufferVk, &offset);

        const VkIndexType indexType = VK_INDEX_TYPE_UINT32;
        const Buffer* indexBuffer = mResourceManager->GetBuffer(false);
        vkCmdBindIndexBuffer(cmdBuffer, indexBuffer->GetBuffer(), 0, indexType);

        const VkDescriptorSet descriptorSet = mFrameContext->GetCommandDescriptorSetInFlight();
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DescriptorLayout::GetPipelineLayout(eLayoutType::Common), 0, 1, &descriptorSet, 0, nullptr);
    }

    void Renderer::DrawIndexed(uint32_t indexCount, uint32_t vertexOffset, uint32_t indexOffset)
    {
        const VkCommandBuffer cmdBuffer = mFrameContext->GetCommandBufferInFlight();
        vkCmdDrawIndexed(cmdBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
    }

    void Renderer::WaitForLastFrame() const
    {
        mFrameContext->WaitForLastFrame();
        mFrameContext->ResetFence();
    }

    void Renderer::BeginRender()
    {
        const VkCommandBuffer cmdBuffer = mFrameContext->GetCommandBufferInFlight();

        mCurImageIndex = mFrameContext->AcquireSwapchainImage(mSwapChain);

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        // MEMO: 암묵적 초기화
        const VkResult result = vkBeginCommandBuffer(cmdBuffer, &commandBufferBeginInfo);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "vkBeginCommandBuffer 실패 resultCode(%d)", result);
        }

        Uploader::TransitionImageLayout(cmdBuffer, mImages[mCurImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT , VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        Uploader::TransitionImageLayout(cmdBuffer, mDepthStencil->GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

        constexpr VkClearValue ClearColorValue{0.4f, 0.8f, 0.6f, 1.0f};
        VkClearValue ClearDepthStencilValue;

        VkRenderingAttachmentInfo renderingAttachmentInfo{};
        renderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        renderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        renderingAttachmentInfo.imageView = mImageViews[mCurImageIndex];
        renderingAttachmentInfo.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        renderingAttachmentInfo.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
        renderingAttachmentInfo.clearValue = ClearColorValue;

        ClearDepthStencilValue.depthStencil = VkClearDepthStencilValue{ 1.0f, 0 };
        constexpr VkImageSubresourceRange subresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };

        VkRenderingAttachmentInfo renderingAttachmentInfoDepth{};
        renderingAttachmentInfoDepth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        renderingAttachmentInfoDepth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        renderingAttachmentInfoDepth.imageView = mDepthStencilView;
        renderingAttachmentInfoDepth.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        renderingAttachmentInfoDepth.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
        renderingAttachmentInfoDepth.clearValue = ClearDepthStencilValue;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.pColorAttachments = &renderingAttachmentInfo;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pDepthAttachment = &renderingAttachmentInfoDepth;
        renderingInfo.layerCount = 1;
        renderingInfo.renderArea = { {0, 0}, mSwapChainExtent };
        vkCmdBeginRendering(cmdBuffer, &renderingInfo);
    }

    void Renderer::EndRender()
    {
        const VkCommandBuffer cmdBuffer = mFrameContext->GetCommandBufferInFlight();

        vkCmdEndRendering(cmdBuffer);

        Uploader::TransitionImageLayout(cmdBuffer, mImages[mCurImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        vkEndCommandBuffer(cmdBuffer);
    }

    void Renderer::SubmitCommands() const
    {
        const VkCommandBuffer cmdBuffer = mFrameContext->GetCommandBufferInFlight();

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
        commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferSubmitInfo.commandBuffer = cmdBuffer;

        VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo{};
        signalSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalSemaphoreSubmitInfo.semaphore = mRenderFinishedSemaphores[mCurImageIndex];
        signalSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
        signalSemaphoreSubmitInfo.value = 1;

        VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo{};
        waitSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        waitSemaphoreSubmitInfo.semaphore = mFrameContext->GetImageReadySemaphoreInFlight();
        waitSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
        waitSemaphoreSubmitInfo.value = 1;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &waitSemaphoreSubmitInfo;
        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphoreSubmitInfo;
        submitInfo.signalSemaphoreInfoCount = 1;
        vkQueueSubmit2(mQueue, 1, & submitInfo, mFrameContext->GetRenderFinishedSemaphoreInFlight());
    }

    void Renderer::Present() const
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pSwapchains = &mSwapChain;
        presentInfo.swapchainCount = 1;
        presentInfo.pImageIndices = &mCurImageIndex;

        presentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[mCurImageIndex];
        presentInfo.waitSemaphoreCount = 1;

        vkQueuePresentKHR(mQueue, &presentInfo);
    }

    void Renderer::CountFrame() const
    {
        mFrameContext->CountFrame();
    }

    void Renderer::WaitForDevice() const
    {
        const VkResult result = vkDeviceWaitIdle(mDevice);
        ASSERT(result == VK_SUCCESS, "vkDeviceWaitIdle 실패. result(%u)", result);
    }

    void Renderer::UpdateUniformData(const std::vector<renderer::UniformWriteInfo>& uniformWriteInfos) const
    {
        const uint32_t nowInFlightIndex = mFrameContext->GetNowInFlightIndex();
        for(const auto& writeInfo : uniformWriteInfos)
        {
            writeInfo.TargetSet->UpdateUniformData(writeInfo.Data, 1, writeInfo.Range.Count, nowInFlightIndex);
        }
    }

    void Renderer::UploadPushConstant(const PushConstantBlock& dataBlock, eShader shaderType) const
    {
        VkCommandBuffer cmdBuf = mFrameContext->GetCommandBufferInFlight();
        VkPushConstantsInfo pushConstantsInfo{};
        pushConstantsInfo.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
        pushConstantsInfo.offset = 0;
        pushConstantsInfo.size = sizeof(PushConstantBlock);
        const eLayoutType layoutType = DescriptorLayout::GetLayoutTypeByShaderType(shaderType);
        pushConstantsInfo.layout = DescriptorLayout::GetPipelineLayout(layoutType);
        pushConstantsInfo.pValues = &dataBlock;
        pushConstantsInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        vkCmdPushConstants2(cmdBuf, &pushConstantsInfo);
    }

    void Renderer::BindDescriptorSet(const DescriptorSet* const descriptorSet)
    {
        const VkCommandBuffer cmdBuffer = mFrameContext->GetCommandBufferInFlight();
        const uint32_t nowInFlightIndex = mFrameContext->GetNowInFlightIndex();

        const eLayoutType layoutType = descriptorSet->GetLayoutType();
        const uint32_t setBindingSlot = 1;
        const VkDescriptorSet descSet = descriptorSet->GetDescriptorSet(nowInFlightIndex);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DescriptorLayout::GetPipelineLayout(layoutType), setBindingSlot, 1, &descSet, 0, nullptr);
    }

    void Renderer::BindPipeline(HashID hash) const
    {
        const VkCommandBuffer cmdBuffer = mFrameContext->GetCommandBufferInFlight();

        const VkPipeline pipeline = mResourceManager->TryGetPipeline(hash);
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    bool Renderer::TryCreateGraphicsPipeline(HashID& outPipelineHash, eShader shader, eVertexFormat vertexFormat, ePrimitiveTopology topology, eRasterType rasterType, eBlendState blendState, eDepthStencilState depthStencilState, eRenderPass renderPass)
    {
        PipelineBuilder pipelineBuilder(mDevice);
        const HashID peekHash = pipelineBuilder.PeekPipelineHash(shader, vertexFormat, topology, rasterType, blendState, depthStencilState, renderPass);
        if(mResourceManager->TryGetPipeline(peekHash))
        {
            outPipelineHash = peekHash;
            return true;
        }
        pipelineBuilder.SetColorBlendState(blendState);
        pipelineBuilder.SetDynamicState(true, true);
        pipelineBuilder.SetDepthStencilState(depthStencilState);
        pipelineBuilder.SetInputAssemblyState(topology);
        pipelineBuilder.SetMultiSamplerState(false);
        pipelineBuilder.SetRasterizationState(rasterType);
        pipelineBuilder.SetRenderPass(renderPass);
        pipelineBuilder.SetVertexInputState(vertexFormat);
        pipelineBuilder.SetShaderStage(*mResourceManager->GetShaderByType(shader));
        pipelineBuilder.SetViewportState(mSwapChainExtent.width, mSwapChainExtent.height);
        const eLayoutType layoutType = DescriptorLayout::GetLayoutTypeByShaderType(shader);
        pipelineBuilder.SetPipelineLayout(DescriptorLayout::GetPipelineLayout(layoutType));

        std::vector<VkFormat> formats(1, mSurfaceFormat.format);
        pipelineBuilder.SetColorAttachInfo(formats, mDepthStencilFormat);
        VkPipeline pipeline;
        if(pipelineBuilder.Create(pipeline, outPipelineHash) == false)
        {
            return false;
        }

        mResourceManager->AddPipeline(outPipelineHash, pipeline);
        return true;
    }

    ResourceManager* Renderer::GetResourceManager() const
    {
        return mResourceManager;
    }

    DescriptorAllocator* Renderer::GetDescriptorAllocator() const
    {
        return mDescriptorAllocator;
    }

    VkDevice Renderer::GetDevice() const
    {
        return mDevice;
    }

    bool Renderer::createInstance(const char* const appName)
    {
        VkResult result = VK_SUCCESS;
        constexpr const char* const RequiredLayers[] =
        {
#ifdef _DEBUG
            "VK_LAYER_KHRONOS_validation",
#endif
        };

        std::vector<const char*> instanceLayers(std::begin(RequiredLayers), std::end(RequiredLayers));
        instanceLayers.push_back(nullptr);

        std::vector<VkExtensionProperties> extensions;
        for (const auto& layerName : instanceLayers)
        {
            const uint32_t extensionCount = extensions.size();
            uint32_t layerExtensionCount = 0;
            result = vkEnumerateInstanceExtensionProperties(layerName, &layerExtensionCount, nullptr);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "Instance Extensions 쿼리 실패 resultCode(%d)", result);
            }

            extensions.resize(extensionCount + layerExtensionCount);
            result = vkEnumerateInstanceExtensionProperties(layerName, &layerExtensionCount, &extensions[extensionCount]);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "Instance Extensions 구성 실패 resultCode(%d)", result);
            }
        }

        std::vector<const char*> requiredExtensionNames;
        requiredExtensionNames.reserve(extensions.size());

        for (const auto& extension : extensions)
        {
            requiredExtensionNames.push_back(extension.extensionName);
        }
        
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = appName;
        appInfo.pEngineName = appName;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_4;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = requiredExtensionNames.size();
        createInfo.ppEnabledExtensionNames = requiredExtensionNames.data();
        createInfo.enabledLayerCount = std::size(RequiredLayers);
        createInfo.ppEnabledLayerNames = RequiredLayers;

        result = vkCreateInstance(&createInfo, nullptr, &mVkInstance);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "Instance 생성 실패 resultCode(%d)", result);
            return false;
        }

        return true;
    }

    bool Renderer::createSurface(HWND handleWindow, HINSTANCE handleInstance)
    {
        VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo{};
        win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        win32SurfaceCreateInfo.hwnd = handleWindow;
        win32SurfaceCreateInfo.hinstance = handleInstance;

        const VkResult result = vkCreateWin32SurfaceKHR(mVkInstance, &win32SurfaceCreateInfo, nullptr, &mSurface);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "surface(Win32) 생성 실패 resultCode(%d)", result);
            return false;
        }

        return true;
    }

    bool Renderer::choosePhysicalDevice()
    {
        uint32_t physicalDeviceCount = 0;
        VkResult result = vkEnumeratePhysicalDevices(mVkInstance, &physicalDeviceCount, nullptr);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "PhysicalDevices 쿼리 실패 resultCode(%d)", result);
        }

        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        result = vkEnumeratePhysicalDevices(mVkInstance, &physicalDeviceCount, physicalDevices.data());
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "PhysicalDevices 구성 실패 resultCode(%d)", result);
        }

        constexpr uint32_t DeviceTypeScoreTable[] =
        {
            0,
            3000,
            4000,
            2000,
            1000,
        };
        uint32_t highScore = 0;
        bool bSupportGraphicsQueueChosenDevice = false;
        for (const auto& physicalDevice : physicalDevices)
        {
            uint32_t score = 0;
            // MEMO: App이 선호하는 GPU 정보 및 요구하는 API 버전 정보를 확인하는 부분
            VkPhysicalDeviceProperties2 deviceProperties2{};
            deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);

            score = DeviceTypeScoreTable[deviceProperties2.properties.deviceType];
            score += deviceProperties2.properties.apiVersion > VK_API_VERSION_1_3 ? 1000 : 0;
            score += deviceProperties2.properties.apiVersion > VK_API_VERSION_1_4 ? 1000 : 0;
            
            // MEMO: 다 얻어오기보다, 필수 요구사항을 검사할 대상들만 얻어와 처리하도록 한다
            VkPhysicalDeviceVulkan11Features deviceFeatures11{};
            deviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            VkPhysicalDeviceVulkan12Features deviceFeatures12{};
            deviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            deviceFeatures12.pNext = &deviceFeatures11;
            VkPhysicalDeviceVulkan13Features deviceFeatures13{};
            deviceFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            deviceFeatures13.pNext = &deviceFeatures12;

            VkPhysicalDeviceFeatures2 deviceFeatures{};
            deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            deviceFeatures.pNext = &deviceFeatures13;
            vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);

            if (deviceFeatures12.bufferDeviceAddress == false)
            {
                continue;
            }

            if (deviceFeatures12.descriptorIndexing == false)
            {
                continue;
            }

            if (deviceFeatures13.dynamicRendering == false)
            {
                continue;
            }

            if (deviceFeatures13.synchronization2 == false)
            {
                continue;
            }

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, nullptr);
            VkQueueFamilyProperties2 queueFamilyProperty{};
            queueFamilyProperty.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
            std::vector<VkQueueFamilyProperties2> queueFamilyProperties(queueFamilyCount, queueFamilyProperty);

            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

            bool bSupportGraphicsQueue = false;
            VkBool32 isSupportSurface = VK_FALSE;
            for (uint32_t queueFamily = 0; queueFamily < queueFamilyProperties.size(); ++queueFamily)
            {
                if (queueFamilyProperties[queueFamily].queueFamilyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT)
                {
                    result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamily, mSurface, &isSupportSurface);
                    if (result != VK_SUCCESS)
                    {
                        ASSERT(false, "SurfaceSupport 조회 실패 resultCode(%d)", result);
                    }
                    bSupportGraphicsQueue = true;
                }
            }

            if (bSupportGraphicsQueue == false || isSupportSurface == VK_FALSE)
            {
                continue;
            }

            if (score > highScore)
            {
                highScore = score;
                mPhysicalDevice = physicalDevice;
                mPhysicalDeviceProperties = std::move(deviceProperties2);
                bSupportGraphicsQueueChosenDevice = bSupportGraphicsQueue;
            }
        }

        if (bSupportGraphicsQueueChosenDevice == false)
        {
            ASSERT(false, "선택된 GPU가 GraphicsQueue를 지원하지 않습니다.");
            return false;
        }

        if (mPhysicalDeviceProperties.properties.apiVersion < VK_API_VERSION_1_3)
        {
            ASSERT(false, "선택된 GPU가 요구하는 버전을 지원하지 않습니다.");
            return false;
        }

        VkHelper::SetSupportStagingUpload(mPhysicalDeviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
        return true;
    }

    bool Renderer::createLogicalDevice()
    {
        uint32_t graphicsQueueIndex = UINT32_MAX;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(mPhysicalDevice, &queueFamilyCount, nullptr);

        VkQueueFamilyProperties2 queueFamilyProperty{};
        queueFamilyProperty.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        std::vector<VkQueueFamilyProperties2> queueFamilyProperties(queueFamilyCount, queueFamilyProperty);
        vkGetPhysicalDeviceQueueFamilyProperties2(mPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        for (uint32_t queueFamily = 0; queueFamily < queueFamilyProperties.size(); ++queueFamily)
        {
            if (queueFamilyProperties[queueFamily].queueFamilyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT)
            {
                graphicsQueueIndex = queueFamily;
            }
        }
        ASSERT(graphicsQueueIndex < UINT32_MAX, "QueueFamilyIndex 가 유효하지 않습니다.");

        uint32_t deviceExtensionCount = 0;
        VkResult result = vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &deviceExtensionCount, nullptr);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "PhysicalDevices DeviceExtension 쿼리 실패 resultCode(%d)", result);
        }

        std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
        result = vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data());
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "PhysicalDevices DeviceExtension 구성 실패 resultCode(%d)", result);
        }

        std::vector<const char*> requiredDeviceExtensionNames;
        requiredDeviceExtensionNames.reserve(deviceExtensionCount);

        for (const auto& extension : deviceExtensions)
        {
            if (strcmp(extension.extensionName, VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0)
            {
                continue;
            }
            requiredDeviceExtensionNames.push_back(extension.extensionName);
        }
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
        queueCreateInfo.queueCount = 1;
        float queuePriority = 0.5f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        mGraphicsQueueIndex = graphicsQueueIndex;

        VkPhysicalDeviceVulkan11Features deviceFeatures11{};
        deviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        VkPhysicalDeviceVulkan12Features deviceFeatures12{};
        deviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        deviceFeatures12.pNext = &deviceFeatures11;
        VkPhysicalDeviceVulkan13Features deviceFeatures13{};
        deviceFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        deviceFeatures13.pNext = &deviceFeatures12;
    	VkPhysicalDeviceVulkan14Features deviceFeatures14{};
        deviceFeatures14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
        deviceFeatures14.pNext = &deviceFeatures13;

        VkPhysicalDeviceFeatures2 deviceFeatures{};
        deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures.pNext = &deviceFeatures14;
        vkGetPhysicalDeviceFeatures2(mPhysicalDevice, &deviceFeatures);


        VkDeviceCreateInfo deviceCreateInfo{
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            &deviceFeatures,
            0,
            1,
            &queueCreateInfo,
            0,
            nullptr,
            static_cast<uint32_t>(requiredDeviceExtensionNames.size()),
            requiredDeviceExtensionNames.data(),
            nullptr
        };

        result = vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "device 생성 실패 resultCode(%d)", result);
            return false;
        }

        VkDeviceQueueInfo2 deviceQueueInfo{};
        deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
        deviceQueueInfo.queueFamilyIndex = graphicsQueueIndex;
        deviceQueueInfo.queueIndex = graphicsQueueIndex;

        vkGetDeviceQueue2(mDevice, &deviceQueueInfo, &mQueue);
        return true;
    }

    bool Renderer::createSwapChain(uint16_t width, uint16_t height)
    {
        VkSurfaceCapabilities2KHR surfaceCapabilities{};
        surfaceCapabilities.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;

        VkPhysicalDeviceSurfaceInfo2KHR deviceSurfaceInfo{};
        deviceSurfaceInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
        deviceSurfaceInfo.surface = mSurface;

        VkResult result = vkGetPhysicalDeviceSurfaceCapabilities2KHR(mPhysicalDevice, &deviceSurfaceInfo, &surfaceCapabilities);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "SurfaceCapabilities2 쿼리 실패 resultCode(%d)", result);
            return false;
        }

        uint32_t surfaceFormatCount = 0;
        result = vkGetPhysicalDeviceSurfaceFormats2KHR(mPhysicalDevice, &deviceSurfaceInfo, &surfaceFormatCount, nullptr);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "SurfaceFormats2 쿼리 실패 resultCode(%d)", result);
            return false;
        }

        VkSurfaceFormat2KHR surfaceFormat{};
        surfaceFormat.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        std::vector<VkSurfaceFormat2KHR> surfaceFormats(surfaceFormatCount, surfaceFormat);
        result = vkGetPhysicalDeviceSurfaceFormats2KHR(mPhysicalDevice, &deviceSurfaceInfo, &surfaceFormatCount, surfaceFormats.data());
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "SurfaceFormats2 구성 실패 resultCode(%d)", result);
            return false;
        }

        uint32_t surfacePresentModeCount = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &surfacePresentModeCount, nullptr);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "SurfacePresentModes 쿼리 실패 resultCode(%d)", result);
            return false;
        }

        std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &surfacePresentModeCount, surfacePresentModes.data());
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "SurfacePresentModes 구성 실패 resultCode(%d)", result);
            return false;
        }

        VkSwapchainCreateInfoKHR swapChainCreateInfo{};
        swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainCreateInfo.surface = mSurface;

        for (const auto& format : surfaceFormats)
        {
            // MEMO: preferred format
            if (format.surfaceFormat.format == VkFormat::VK_FORMAT_R8G8B8A8_SRGB && format.surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
            {
                swapChainCreateInfo.imageFormat = format.surfaceFormat.format;
                swapChainCreateInfo.imageColorSpace = format.surfaceFormat.colorSpace;
            }
        }
        mSurfaceFormat.format = swapChainCreateInfo.imageFormat;
        mSurfaceFormat.colorSpace = swapChainCreateInfo.imageColorSpace;

        swapChainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& mode : surfacePresentModes)
        {
            // MEMO: preferred mode
            if (mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
            {
                swapChainCreateInfo.presentMode = mode;
            }
        }

        if (surfaceCapabilities.surfaceCapabilities.currentExtent.width == UINT_MAX && surfaceCapabilities.surfaceCapabilities.currentExtent.height == UINT_MAX)
        {
            swapChainCreateInfo.imageExtent.width = std::clamp(static_cast<uint32_t>(width), surfaceCapabilities.surfaceCapabilities.minImageExtent.width, surfaceCapabilities.surfaceCapabilities.maxImageExtent.width);
            swapChainCreateInfo.imageExtent.height = std::clamp(static_cast<uint32_t>(height), surfaceCapabilities.surfaceCapabilities.minImageExtent.height, surfaceCapabilities.surfaceCapabilities.maxImageExtent.height);
        }
        else
        {
            swapChainCreateInfo.imageExtent = surfaceCapabilities.surfaceCapabilities.currentExtent;
        }

        swapChainCreateInfo.minImageCount = (surfaceCapabilities.surfaceCapabilities.maxImageCount > 0) ? surfaceCapabilities.surfaceCapabilities.maxImageCount : 3;
        swapChainCreateInfo.minImageCount = (swapChainCreateInfo.minImageCount > surfaceCapabilities.surfaceCapabilities.maxImageCount) ? surfaceCapabilities.surfaceCapabilities.maxImageCount : swapChainCreateInfo.minImageCount;

        mSwapChainExtent = swapChainCreateInfo.imageExtent;

        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapChainCreateInfo.imageSharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.preTransform = surfaceCapabilities.surfaceCapabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapChainCreateInfo.clipped = VK_TRUE;
        swapChainCreateInfo.oldSwapchain = nullptr;

        result = vkCreateSwapchainKHR(mDevice, &swapChainCreateInfo, nullptr, &mSwapChain);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "SwapChain 생성 실패 resultCode(%d)", result);
            return false;
        }

        ASSERT(mImages.empty(), "images가 비어있지 않습니다. (리소스를 재생성하고 있는 경우 처리가 필요)");
        uint32_t imageCount = 0;
        result = vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, nullptr);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "SwapChain Images 쿼리 실패 resultCode(%d)", result);
            return false;
        }

        mImages.resize(imageCount);
        result = vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, mImages.data());
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "SwapChain Images 구성 실패 resultCode(%d)", result);
            return false;
        }

        return true;
    }

    bool Renderer::createImageViews()
    {
        ASSERT(mImageViews.empty(), "image views가 비어있지 않습니다. (리소스를 재생성하고 있는 경우 처리가 필요)");
        mImageViews.reserve(mImages.size());

        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = mSurfaceFormat.format;
        viewCreateInfo.components = {}; // VK_COMPONENT_SWIZZLE_IDENTITY
        viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        for (const auto& image : mImages)
        {
            viewCreateInfo.image = image;

            VkImageView imageView{};
            const VkResult result = vkCreateImageView(mDevice, &viewCreateInfo, nullptr, &imageView);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "ImageView 생성 실패 resultCode(%d)", result);
                continue;
            }
            mImageViews.push_back(std::move(imageView));
        }
        return true;
    }

    bool Renderer::createSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        mRenderFinishedSemaphores.resize(mImages.size());
        for (auto& semaphore: mRenderFinishedSemaphores)
        {
            const VkResult result = vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &semaphore);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "Semaphore(RenderFinished) 생성 실패 resultCode(%d)", result);
                return false;
            }
        }
        return true;
    }

    bool Renderer::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flag, VkCommandPool& outCommandPool) const
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
        commandPoolCreateInfo.flags = flag;

        const VkResult result = vkCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &outCommandPool);
        if(result != VK_SUCCESS)
        {
            ASSERT(false, "CommandPool 생성 실패 resultCode(%d)", result);
            return false;
        }
        return true;
    }

    bool Renderer::createCommandBuffers(VkCommandPool commandPool, uint32_t bufferCount, VkCommandBufferLevel level, VkCommandBuffer& outBuffers) const
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.commandBufferCount = bufferCount;
        commandBufferAllocateInfo.level = level;

        const VkResult result = vkAllocateCommandBuffers(mDevice, &commandBufferAllocateInfo, &outBuffers);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "CommandBuffer 생성 실패 resultCode(%d)", result);
            return false;
        }
        return true;
    }

    bool Renderer::createDepthStencil()
    {
        const VkFormat depthStencilFormat = VkHelper::GetSupportImageFormat({ VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        if(depthStencilFormat == VK_FORMAT_MAX_ENUM)
        {
            ASSERT(false, "DepthStencilView 생성 중 포멧 선정 실패. returnFormat(%d)", depthStencilFormat);
            return false;
        }
        mDepthStencil = new Image(mDevice);
        mDepthStencil->Create(VK_IMAGE_LAYOUT_UNDEFINED, mGraphicsQueueIndex, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, { mSwapChainExtent.width, mSwapChainExtent.height, 1 }, 1, VK_IMAGE_TYPE_2D, depthStencilFormat, {}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.format = depthStencilFormat;
        imageViewCreateInfo.components = {};
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.image = mDepthStencil->GetImage();
        imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };
        const VkResult result = vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mDepthStencilView);
        if(result != VK_SUCCESS)
        {
            ASSERT(false, "DepthStencilView 생성 실패. result(%d)", result);
            return false;
        }
        mDepthStencilFormat = depthStencilFormat;
        return true;
    }
}
