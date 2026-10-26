#include "Renderer.h"
#include <algorithm>
#include "DescriptorSet.h"
#include "FrameContext.h"
#include "Uploader.h"
#include "VkHelper.h"
#include "../Util/Macro.h"
#include "../Util/Util.h"
#include "Resources/Buffer.h"
#include "Resources/PipelineBuilder.h"
#include "Resources/Sampler.h"
#include "Shader/Shader.h"
#include "Shader/ShaderModule.h"

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
        , mFrameContext(nullptr)
        , mSimplePipeline(nullptr)
        , mVertexBuffer()
        , mIndexBuffer()
    {
    }

    Renderer::~Renderer()
    {
        Uploader::Release();

        const VkResult result = vkDeviceWaitIdle(mDevice);
        ASSERT(result == VK_SUCCESS, "vkDeviceWaitIdle 실패. result(%u)", result);

        for(const auto& textureIt : mTextures)
        {
            vkFreeMemory(mDevice, textureIt.second.ImageMemory, nullptr);
            vkDestroyImageView(mDevice, textureIt.second.ImageView, nullptr);
            vkDestroyImage(mDevice, textureIt.second.Image, nullptr);
        }

        for(const auto& samplerIt : mSamplers)
        {
            delete samplerIt.second;
        }

        for(const auto& shaderIt : mShaders)
        {
            delete shaderIt;
        }
        mShaders.clear();

        for(const auto& shaderModuleIt: mShaderModules)
        {
            delete shaderModuleIt.second;
        }
        mShaderModules.clear();

        delete mVertexBuffer;
        delete mIndexBuffer;

        delete mFrameContext;
        mFrameContext = nullptr;
        vkDestroyCommandPool(mDevice, mCommandPoolTransfer, nullptr);
        vkDestroyPipeline(mDevice, mSimplePipeline, nullptr);


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
        vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
        vkDestroySurfaceKHR(mVkInstance, mSurface, nullptr);
        vkDestroyDevice(mDevice, nullptr);
        vkDestroyInstance(mVkInstance, nullptr);
    }

    bool Renderer::initializeWithVulkan(HWND handleWindow, HINSTANCE handleInstance, int16_t width, int16_t height, const char* const appName)
    {
        mWindowWidth = width;
        mWindowHeight = height;

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

        VkPhysicalDeviceMemoryProperties2 physicalDeviceMemoryProperties2{};
        physicalDeviceMemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

        vkGetPhysicalDeviceMemoryProperties2(mPhysicalDevice, &physicalDeviceMemoryProperties2);
        VkHelper::SetPhysicalDeviceMemoryProperties(physicalDeviceMemoryProperties2.memoryProperties);


        if(createLogicalDevice() == false)
        {
            return false;
        }
        VkHelper::SetQueueFamilyIndices(mGraphicsQueueIndex, UINT32_MAX);

        if(createSwapChain(width, height) == false)
        {
            return false;
        }

        if (createImageViews() == false)
        {
            return false;
        }

        if(createSyncObjects() == false)
        {
            return false;
        }

        if(createShaders() == false)
        {
            return false;
        }

        mFrameContext = new FrameContext(mDevice);
        if(mFrameContext->Initialize(sOverlappedFrame, mShaders) == false)
        {
            return false;
        }

        if(createSamplers() == false)
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

        Uploader::Initialize(mDevice, mCommandBufferTransfer, mCommandPoolTransfer, mQueue, mGraphicsQueueIndex);
        return true;
    }

    void Renderer::PrepareObjects()
    {
        mVertexBuffer = new Buffer(mDevice);
        mVertexBuffer->Create(771336 * 2, Buffer::eBufferType::Vertex, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), mGraphicsQueueIndex);
        mIndexBuffer = new Buffer(mDevice);
        mIndexBuffer->Create(189792 * 2, Buffer::eBufferType::Index, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), mGraphicsQueueIndex);
    }

    void Renderer::UpdateScene(std::vector<std::pair<renderer::eCbType, void*>>& uniforms)
    {
        for(const auto& uniform : uniforms)
        {
            mFrameContext->UpdateUniformData(eShader::Simple, uniform.second, 1, sizeof(CbViewProj), uniform.first == eCbType::CbViewProj);
        }
        const bool result = mFrameContext->UpdateDescriptorSet(eShader::Simple);
        ASSERT(result , "UpdateDescriptorSet 실패");
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


        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSimplePipeline);

        const VkDeviceSize offset = 0;
        const VkBuffer vertexBuffer = mVertexBuffer->GetBuffer();
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, &offset);

        const VkIndexType indexType = VK_INDEX_TYPE_UINT32;
        vkCmdBindIndexBuffer(cmdBuffer, mIndexBuffer->GetBuffer(), 0, indexType);

        const VkDescriptorSet descriptorSet = mFrameContext->GetCommandDescriptorSetInFlight(eShader::Simple);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mShaders[static_cast<EnumInt>(eShader::Simple)]->GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
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

        transitionImageLayout(cmdBuffer, mCurImageIndex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT , VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

        constexpr VkClearValue ClearColorValue{0.4f, 0.8f, 0.6f, 1.0f};
        constexpr VkImageSubresourceRange subresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };

        VkRenderingAttachmentInfo renderingAttachmentInfo{};
        renderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        renderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        renderingAttachmentInfo.imageView = mImageViews[mCurImageIndex];
        renderingAttachmentInfo.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        renderingAttachmentInfo.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
        renderingAttachmentInfo.clearValue = ClearColorValue;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.pColorAttachments = &renderingAttachmentInfo;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.layerCount = 1;
        renderingInfo.renderArea = { {0, 0}, mSwapChainExtent };
        vkCmdBeginRendering(cmdBuffer, &renderingInfo);
    }

    void Renderer::EndRender()
    {
        const VkCommandBuffer cmdBuffer = mFrameContext->GetCommandBufferInFlight();

        vkCmdEndRendering(cmdBuffer);

        transitionImageLayout(cmdBuffer, mCurImageIndex, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
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

    bool Renderer::TryCreateGraphicsPipeline(HashID& outPipelineHash, eShader shader, eVertexFormat vertexFormat, ePrimitiveTopology topology, eRasterType rasterType, eBlendState blendState, eDepthStencilState depthStencilState, eRenderPass renderPass)
    {
        PipelineBuilder pipelinevBuilder(mDevice);
        pipelinevBuilder.SetColorBlendState(blendState);
        pipelinevBuilder.SetDynamicState(true, true);
        pipelinevBuilder.SetDepthStencilState(depthStencilState);
        pipelinevBuilder.SetInputAssemblyState(topology);
        pipelinevBuilder.SetMultiSamplerState(false);
        pipelinevBuilder.SetRasterizationState(rasterType);
        pipelinevBuilder.SetRenderPass(renderPass);
        pipelinevBuilder.SetVertexInputState(vertexFormat);
        pipelinevBuilder.SetShaderStage(*mShaders[static_cast<EnumInt>(shader)]);
        pipelinevBuilder.SetViewportState(mSwapChainExtent.width, mSwapChainExtent.height);
        pipelinevBuilder.SetPipelineLayout(mShaders[static_cast<EnumInt>(shader)]->GetPipelineLayout());

        std::vector<VkFormat> formats(1, mSurfaceFormat.format);
        pipelinevBuilder.SetColorAttachInfo(formats);

        return pipelinevBuilder.Create(mSimplePipeline, outPipelineHash);
    }

    void Renderer::AddVertexData(const void* const pData, uint32_t attribSize, uint32_t count, HashID hash, BufferRange& outRange)
    {
        mVertexBuffer->AddData(pData, count, attribSize, hash, outRange);
    }

    void Renderer::AddIndexData(void* pData, uint32_t attribSize, uint32_t count, HashID hash, BufferRange& outRange)
    {
        mIndexBuffer->AddData(pData, count, attribSize, hash, outRange);
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

    bool Renderer::createSamplers()
    {
        for(uint32_t samplerType = 0; samplerType < static_cast<uint32_t>(eSamplerType::SamplerCount); ++samplerType)
        {
            Sampler* sampler = new Sampler(mDevice);
            if(sampler->Create(static_cast<eSamplerType>(samplerType)) == false)
            {
                return false;
            }

            mSamplers.insert({ static_cast<eSamplerType>(samplerType), sampler});
        }
        return true;
    }

    bool Renderer::createShaders()
    {
        struct ShaderStageHash
        {
            eShader Type;
            HashID VertexShaderHash;
            HashID PixelShaderHash;
        };

        struct VertexShaderSourceInfo
        {
            eVertexShader Type;
            const char* SourcePath;
        };

        constexpr VertexShaderSourceInfo VertexShaderSourceList[] =
        {
            {eVertexShader::VsOutline, "Renderer/Shader/Precompiled/VsOutline.spv"},
            {eVertexShader::VsBasicWithShadow, "Renderer/Shader/Precompiled/VsBasicWithShadow.spv"},
            {eVertexShader::VsSimple, "Renderer/Shader/Precompiled/VsSimple.spv"},
            {eVertexShader::VsSkybox, "Renderer/Shader/Precompiled/VsSkybox.spv"},
            {eVertexShader::VsTexture, "Renderer/Shader/Precompiled/VsTexture.spv"},
            {eVertexShader::VsScreen, "Renderer/Shader/Precompiled/VsScreen.spv"},
            {eVertexShader::VsShadow, "Renderer/Shader/Precompiled/VsShadow.spv"},
            {eVertexShader::VsDeferred, "Renderer/Shader/Precompiled/VsDeferred.spv"},
            {eVertexShader::VsDebugColor, "Renderer/Shader/Precompiled/VsDebugColor.spv"},
        };
        static_assert(std::size(VertexShaderSourceList) == static_cast<uint8_t>(eVertexShader::VertexShaderCount), "열거형값과 List의 요소가 서로 일치해야 합니다.");


        struct PixelShaderSourceInfo
        {
            ePixelShader Type;
            const char* SourcePath;
        };

        constexpr PixelShaderSourceInfo PixelShaderSourceList[] =
        {
            {ePixelShader::PsOutline, "Renderer/Shader/Precompiled/PsOutline.spv"},
            {ePixelShader::PsBasicWithShadow, "Renderer/Shader/Precompiled/PsBasicWithShadow.spv"},
            {ePixelShader::PsShadow, "Renderer/Shader/Precompiled/PsShadow.spv"},
            {ePixelShader::PsTexture, "Renderer/Shader/Precompiled/PsTexture.spv"},
            {ePixelShader::PsSkybox, "Renderer/Shader/Precompiled/PsSkybox.spv"},
            {ePixelShader::PsColor, "Renderer/Shader/Precompiled/PsColor.spv"},
            {ePixelShader::PsDeferred, "Renderer/Shader/Precompiled/PsDeferred.spv"},
            {ePixelShader::PsTextureMRT, "Renderer/Shader/Precompiled/PsTextureMRT.spv"},
            {ePixelShader::PsSimple, "Renderer/Shader/Precompiled/PsSimple.spv"},
        };
        static_assert(std::size(PixelShaderSourceList) == static_cast<uint8_t>(ePixelShader::PixelShaderCount), "열거형값과 List의 요소가 서로 일치해야 합니다.");

        HashID vsHashes[static_cast<uint8_t>(eVertexShader::VertexShaderCount)];
        for (const VertexShaderSourceInfo shaderInfo : VertexShaderSourceList)
        {
            ShaderModule* shaderModule = new ShaderModule(mDevice);
            HashID shaderHash;
            if(shaderModule->Create(shaderInfo.SourcePath, eShaderStageType::Vertex, shaderHash) == false)
            {
                return false;
            }
            mShaderModules.insert({shaderHash, shaderModule});
            vsHashes[static_cast<EnumInt>(shaderInfo.Type)] = shaderHash;
        }

        HashID psHashes[static_cast<uint8_t>(ePixelShader::PixelShaderCount)];
        for (const PixelShaderSourceInfo shaderInfo : PixelShaderSourceList)
        {
            ShaderModule* shaderModule = new ShaderModule(mDevice);
            HashID shaderHash;
            if(shaderModule->Create(shaderInfo.SourcePath, eShaderStageType::Pixel, shaderHash) == false)
            {
                return false;
            }
            mShaderModules.insert({ shaderHash, shaderModule });
            psHashes[static_cast<EnumInt>(shaderInfo.Type)] = shaderHash;
        }

        // construct shader mapping table
        const ShaderStageHash ShaderMapTable[] =
        {
            {eShader::Outline, vsHashes[static_cast<uint8_t>(eVertexShader::VsOutline)], psHashes[static_cast<uint8_t>(ePixelShader::PsOutline)]},
            {eShader::Skybox, vsHashes[static_cast<uint8_t>(eVertexShader::VsSkybox)], psHashes[static_cast<uint8_t>(ePixelShader::PsSkybox)]},
            { eShader::Shadow, vsHashes[static_cast<uint8_t>(eVertexShader::VsShadow)], psHashes[static_cast<uint8_t>(ePixelShader::PsShadow)]},
            {eShader::BasicWithShadow,  vsHashes[static_cast<uint8_t>(eVertexShader::VsBasicWithShadow)], psHashes[static_cast<uint8_t>(ePixelShader::PsBasicWithShadow)]},
            {eShader::Texture,  vsHashes[static_cast<uint8_t>(eVertexShader::VsTexture)], psHashes[static_cast<uint8_t>(ePixelShader::PsTextureMRT)]},
            {eShader::Color,  vsHashes[static_cast<uint8_t>(eVertexShader::VsSimple)], psHashes[static_cast<uint8_t>(ePixelShader::PsColor)]},
            {eShader::DebugHUD,  vsHashes[static_cast<uint8_t>(eVertexShader::VsScreen)], psHashes[static_cast<uint8_t>(ePixelShader::PsTexture)]},
            {eShader::Deferred,  vsHashes[static_cast<uint8_t>(eVertexShader::VsDeferred)], psHashes[static_cast<uint8_t>(ePixelShader::PsDeferred)]},
            {eShader::DebugColor,  vsHashes[static_cast<uint8_t>(eVertexShader::VsDebugColor)], psHashes[static_cast<uint8_t>(ePixelShader::PsColor)]},
            {eShader::Simple,  vsHashes[static_cast<uint8_t>(eVertexShader::VsSimple)], psHashes[static_cast<uint8_t>(ePixelShader::PsSimple)]},
        };
        static_assert(std::size(ShaderMapTable) == static_cast<uint8_t>(eShader::ShaderCount), "열거형값과 ShaderMapTable의 요소가 서로 일치해야 합니다.");

        for (const auto& shaderEntryIt : ShaderMapTable)
        {
            const auto& vsModule = mShaderModules.find(shaderEntryIt.VertexShaderHash);
            ASSERT(vsModule != mShaderModules.end(), "Vs Module이 제대로 초기화되지 않았습니다. type(%d)", static_cast<EnumInt>(shaderEntryIt.Type));
            const auto& psModule = mShaderModules.find(shaderEntryIt.PixelShaderHash);
            ASSERT(psModule != mShaderModules.end(), "Ps Module이 제대로 초기화되지 않았습니다. type(%d)", static_cast<EnumInt>(shaderEntryIt.Type));
            Shader* shader = new Shader(mDevice);
            if(shader->Create(shaderEntryIt.Type, vsModule->second, psModule->second) == false)
            {
                return false;
            }
            mShaders.push_back(shader);
        }

        return true;
    }

    void Renderer::transitionImageLayout(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask)
    {
        VkImageMemoryBarrier2 imageBarrier{};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarrier.image = mImages[imageIndex];

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

    void Renderer::transitionImageLayoutNew(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask)
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
}
