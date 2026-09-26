#include "Renderer.h"
#include <algorithm>
#include "../Util/Macro.h"
#include "Resources/BufferManager.h"
#include "Resources/TextureManager.h"
#include "Shader/ShaderManager.h"

namespace renderer
{

    void Renderer::CheckLiveObjects()
    {
        HMODULE dxgidebugdll = GetModuleHandleW(L"dxgidebug.dll");
        ASSERT(dxgidebugdll != NULL, "dxgidebug.dll 로드 실패");

        decltype(&DXGIGetDebugInterface) GetDebugInterface = reinterpret_cast<decltype(&DXGIGetDebugInterface)>(GetProcAddress(dxgidebugdll, "DXGIGetDebugInterface"));

        IDXGIDebug* debug;

        GetDebugInterface(IID_PPV_ARGS(&debug));

        OutputDebugStringW(L"========================== Direct3D Object ref count 메모리 누수 체크 ===============================\r\n");
        OutputDebugStringW(L"========================== 반환되지 않은 IUnknown 객체가 있을경우 아래에 나타납니다. ============================\r\n");

        debug->ReportLiveObjects(DXGI_DEBUG_D3D11, DXGI_DEBUG_RLO_DETAIL);
        OutputDebugStringW(L"==============================================================================================\r\n");


        debug->Release();
    }

    ID3D11Device* Renderer::GetDevice() const
    {
        return mDevice;
    }

    ID3D11DeviceContext* Renderer::GetDeviceContext() const
    {
        return mDeviceContext;
    }

    void Renderer::GetCurrentPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY& outTopology) const
    {
        mDeviceContext->IAGetPrimitiveTopology(&outTopology);
    }

    BufferManager* const Renderer::GetBufferManager() const
    {
        return mBufferManager;
    }

    eRenderTarget Renderer::GetRenderTargetByRenderPass(eRenderPass renderPass) const
    {
        constexpr eRenderTarget RenderPassRenderTargetMap[] =
        {
            eRenderTarget::GBufferColor,
            eRenderTarget::Shadow,
            eRenderTarget::Default,
            // MEMO: 대표적인 렌더타겟 반환
            eRenderTarget::Default
        };
        static_assert(sizeof(RenderPassRenderTargetMap) / sizeof(RenderPassRenderTargetMap[0]) == static_cast<uint64_t>(eRenderPass::PassCount), "RenderPassRenderTargetMap와 eRenderPass의 갯수가 서로 맞아야 합니다.");
        return RenderPassRenderTargetMap[static_cast<uint8_t>(renderPass)];
    }

    void Renderer::registerSrvTexture()
    {
        mTextureManager->AddTextureByHash(TextureManager::sShadowTexHash, mRenderTargetSRVs[static_cast<uint8_t>(eRenderTarget::Shadow)]);
        TextureManager::sShadowTexSerialID = mTextureManager->GetTextureSerial(TextureManager::sShadowTexHash);

        mTextureManager->AddTextureByHash(TextureManager::sGBufferColorTexHash, mRenderTargetSRVs[static_cast<uint8_t>(eRenderTarget::GBufferColor)]);
        TextureManager::sGBufferColorTexSerialID = mTextureManager->GetTextureSerial(TextureManager::sGBufferColorTexHash);

        mTextureManager->AddTextureByHash(TextureManager::sGBufferNormalTexHash, mRenderTargetSRVs[static_cast<uint8_t>(eRenderTarget::GBufferNormal)]);
        TextureManager::sGBufferNormalTexSerialID = mTextureManager->GetTextureSerial(TextureManager::sGBufferNormalTexHash);

        mTextureManager->AddTextureByHash(TextureManager::sGBufferPositionTexHash, mRenderTargetSRVs[static_cast<uint8_t>(eRenderTarget::GBufferPosition)]);
        TextureManager::sGBufferPositionTexSerialID = mTextureManager->GetTextureSerial(TextureManager::sGBufferPositionTexHash);

        mTextureManager->AddTextureByHash(TextureManager::sGBufferSpecularTexHash, mRenderTargetSRVs[static_cast<uint8_t>(eRenderTarget::GBufferSpecular)]);
        TextureManager::sGBufferSpecularTexSerialID = mTextureManager->GetTextureSerial(TextureManager::sGBufferSpecularTexHash);

        mTextureManager->AddTextureByHash(TextureManager::sGBufferAmbientTexHash, mRenderTargetSRVs[static_cast<uint8_t>(eRenderTarget::GBufferAmbient)]);
        TextureManager::sGBufferAmbientTexSerialID = mTextureManager->GetTextureSerial(TextureManager::sGBufferAmbientTexHash);
    }

    VkRenderer::VkRenderer()
        : mWindowHeight(0)
        , mWindowWidth(0)
        , mVkInstance(nullptr)
        , mPhysicalDevice()
        , mPhysicalDeviceProperties()
        , mDevice(nullptr)
        , mSurface(nullptr)
        , mSwapChain(nullptr)
    {}

    VkRenderer::~VkRenderer()
    {
        mImages.clear();
        std::vector<VkImage>().swap(mImages);
        mImageViews.clear();
        std::vector<VkImageView>().swap(mImageViews);
        vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
        vkDestroySurfaceKHR(mVkInstance, mSurface, nullptr);
        vkDestroyDevice(mDevice, nullptr);
        vkDestroyInstance(mVkInstance, nullptr);
    }

    bool VkRenderer::initializeWithVulkan(HWND handleWindow, HINSTANCE handleInstance, int16_t width, int16_t height, const char* const appName)
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

        if(createLogicalDevice() == false)
        {
            return false;
        }

        if(createSwapChain(width, height) == false)
        {
            return false;
        }

        return true;
    }

    bool VkRenderer::createInstance(const char* const appName)
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

    bool VkRenderer::createSurface(HWND handleWindow, HINSTANCE handleInstance)
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

    bool VkRenderer::choosePhysicalDevice()
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

        return true;
    }

    bool VkRenderer::createLogicalDevice()
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

        return true;
    }

    bool VkRenderer::createSwapChain(uint16_t width, uint16_t height)
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

    Renderer::Renderer()
        : mRefCount(1)
        , mWindowHeight(0)
        , mWindowWidth(0)
        , mDevice(nullptr)
        , mDeviceContext(nullptr)
        , mSwapChain(nullptr)
        , mDepthStencilStates{}
        , mRenderTargetViewList{nullptr}
        , mDepthStencilViewList{nullptr}
        , mRtvDsMapTable{}
        , mRenderTargetSRVs{}
        , mRenderTargetBindDescMap{}
        , mViewportFull()
        , mViewportTex()
        , mRasterStates{nullptr}
        , mSamplerState{}
        , mBlendStates{}
        , mPrimitiveTopologies{}
        , mBufferManager(nullptr)
        , mTextureManager(nullptr)
        , mShaderManager(nullptr)
    {}

    Renderer::~Renderer()
    {
        Cleanup();
        mBufferManager = nullptr;
        mTextureManager = nullptr;
        mShaderManager = nullptr;
#ifdef _DEBUG
        renderer::Renderer::CheckLiveObjects();
#endif
    }

    bool Renderer::createRasterState()
    {
        // 기본 래스터 스테이트
        D3D11_RASTERIZER_DESC rasterDesc;
        ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));

        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        // MEMO: CW winding으로 통일 
        rasterDesc.FrontCounterClockwise = false;
        HRESULT result = mDevice->CreateRasterizerState(&rasterDesc, &mRasterStates[static_cast<uint32_t>(eRasterType::Basic)]);
        if (FAILED(result))
        {
            ASSERT(false, "Failed to create RasterState for basic");
            return false;
        }
        // 아웃라인용 래스터 스테이트
        rasterDesc.CullMode = D3D11_CULL_FRONT;
      //  rasterDesc.CullMode = D3D11_CULL_BACK;
        rasterDesc.DepthBias = 1;
        result = mDevice->CreateRasterizerState(&rasterDesc, &mRasterStates[static_cast<uint32_t>(eRasterType::Outline)]);
        if(FAILED(result))
        {
            ASSERT(false, "Failed to create RasterState for outline");
            return false;
        }

        // 스카이박스용 래스터 스테이트
        rasterDesc.CullMode = D3D11_CULL_BACK;
        result = mDevice->CreateRasterizerState(&rasterDesc, &mRasterStates[static_cast<uint32_t>(eRasterType::Skybox)]);
        if (FAILED(result))
        {
            ASSERT(false, "Failed to create RasterState for Skybox");
            return false;
        }

        // back-culling 래스터 스테이트
        rasterDesc.CullMode = D3D11_CULL_BACK;
        result = mDevice->CreateRasterizerState(&rasterDesc, &mRasterStates[static_cast<uint32_t>(eRasterType::CullBack)]);
        if (FAILED(result))
        {
            ASSERT(false, "Failed to create RasterState for back face culling");
            return false;
        }

        return true;
    }

    HRESULT Renderer::createSamplerState()
    {
        const D3D11_SAMPLER_DESC SamplerDescTable[static_cast<uint8_t>(eSamplerType::SamplerCount)] =
        {
                {D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, 0, 0, D3D11_COMPARISON_NEVER, {}, 0, D3D11_FLOAT32_MAX}
        };

        for (uint8_t sampler = 0; sampler < static_cast<uint8_t>(eSamplerType::SamplerCount); ++sampler)
        {
            if (FAILED(mDevice->CreateSamplerState(&SamplerDescTable[sampler], &mSamplerState[sampler])))
            {
                ASSERT(false, "Failed to create SamplerState. check the desc options. failedIndex(%u)", sampler);
                return E_FAIL;
            }
        }

        return S_OK;
    }

    void Renderer::SetManagers(BufferManager* const bufferManager, TextureManager* const textureManager, ShaderManager* const shaderManager)
    {
        ASSERT(bufferManager, "bufferManager is nullptr");
        ASSERT(textureManager, "textureManager is nullptr");
        ASSERT(shaderManager, "shaderManager is nullptr");
        mBufferManager = bufferManager;
        mTextureManager = textureManager;
        mShaderManager = shaderManager;

        registerSrvTexture();
    }

    HRESULT Renderer::CreateDeviceAndSetup(
        DXGI_SWAP_CHAIN_DESC& swapChainDesc
        , uint32_t              width
        , uint32_t              height
        , bool                bDebugMode)
    {


        D3D_DRIVER_TYPE driverTypes[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE
        };
        UINT32 numDriverTypes = ARRAYSIZE(driverTypes);

        // gpu가 지원하는 최신버전으로 맞춰질 것이다.
        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
        };
        UINT32 numFeatureLevels = ARRAYSIZE(featureLevels);

        UINT32 createDeviceFlag = 0;
        if (bDebugMode)
        {
            createDeviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
        }


        // 왠만하면 D3D_DRIVER_TYPE_HARDWARE로 정해질 것임.
        D3D_FEATURE_LEVEL featureLevel;
        HRESULT result = S_OK;
        for (UINT32 driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex)
        {
            result = D3D11CreateDeviceAndSwapChain(nullptr, driverTypes[driverTypeIndex], nullptr, createDeviceFlag, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &swapChainDesc, &mSwapChain, &mDevice, &featureLevel, &mDeviceContext);
            if(SUCCEEDED(result))
            {
                break;
            }
        }

        if (FAILED(result))
        {
            ASSERT(false, "failed to create device and swapchain");
        }

        SET_PRIVATE_DATA(mDevice, "Renderer::mDevice");
        SET_PRIVATE_DATA(mDeviceContext, "Renderer::mDeviceContext");
        SET_PRIVATE_DATA(mSwapChain, "Renderer::mSwapChain");


        // setup back buffer

        // 백버퍼를 얻어와서 렌더타겟으로 설정하는 부분
        ID3D11Texture2D* backBuffer = nullptr;
        // 데스크탑 버전에서는 기본적으로 스왑체인은 하나의 백버퍼를 가지고 있고, uwp는 만들어줘야한다.
        result = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
        if (FAILED(result))
        {
            return E_FAIL;
        }

        // view가 붙은것과 아닌것의 차이가 있다.
        // 어떤 리소스가 있고 그것을 사용하기 위한 파생 인터페이스. 보통 읽기전용으로 동작한다.
        // 9과달리 11에서는 어떤 원천 리소스를 가지고 어떤 방식으로 사용하냐에 따라 인터페이스가 다 나뉘어 있다고 한다.
        result = CreateRenderTargetView(backBuffer, nullptr, &mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::Default)]);
        // 백버퍼의 역할은 렌더타겟으로 지정해주는 것이 끝이다.
        // refCount이기 때문에 바로 사라지는건 아니고, gRenderTargetView가 백버퍼를 가지기 때문에 이후에 사라질 것.
        backBuffer->Release();

        SET_PRIVATE_DATA(mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::Default)], "eRenderTarget::Default");

        if (FAILED(result))
        {
            return E_FAIL;
        }

        // output merger, 그래픽 버퍼에 써넣는 일을 할 때 OM이 붙음.
        /*
         * 이부분 렌더타겟에 바인딩 되지 않는데 왜 호출했는지 다시 조사 필요함
         */

        mViewportFull.Width = (FLOAT)width;
        mViewportFull.Height = (FLOAT)height;
        // 보통 0~1 값으로 지정한다.
        mViewportFull.MinDepth = 0.0f;
        mViewportFull.MaxDepth = 1.0f;
        mViewportFull.TopLeftX = 0;
        mViewportFull.TopLeftY = 0;
        // 렌더링될 영역을 지정. s가 붙으니까 여러개 지정가능.(모델링 프로그램을 생각)
        // rasterizer stage
        mDeviceContext->RSSetViewports(1, &mViewportFull);

        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;                      // 멀티샘플링 수
        depthDesc.SampleDesc.Quality = 0;                    // 멀티샘플링 퀼리티
        depthDesc.Usage = D3D11_USAGE_DEFAULT;               // 디폴트로 사용
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthDesc.CPUAccessFlags = 0;                        // cpu 액세스 여부
        depthDesc.MiscFlags = 0;
        ID3D11Texture2D* depthStencilTexture = nullptr;
        CreateTexture2D(depthDesc, &depthStencilTexture, "Renderer::depthStencilTexture");

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
        depthStencilViewDesc.Format = depthDesc.Format;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Texture2D.MipSlice = 0;

        result = CreateDepthStencilView(depthStencilTexture, &depthStencilViewDesc, &mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Default)]);
        if (FAILED(result))
        {
            ASSERT(false, "mDepthStencilView 생성 실패");
            return E_FAIL;
        }
        SET_PRIVATE_DATA(mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Default)], "eRenderTarget::Default");
        SAFETY_RELEASE(depthStencilTexture);
        uint32_t index = static_cast<uint8_t>(eRenderTarget::Default);
        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Default)].RenderTargetIndex = index;
        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Default)].DepthStencilIndex = index;
        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Default)].NumViews = 1U;

        // MEMO: 쓰는 것들만 Preset으로 지정한다. 없으면 동적 생성은 굳이 필요하지 않음
        constexpr DepthStencilStateMap DepthStencilStateMap[] =
        {
            // MEMO: DepthOffStencilOff는 unbind용
            {
                eDepthStencilState::DepthOffStencilOff,
                {}
            },
            {
                eDepthStencilState::DepthOnMaskAllCompLessEqual,
                {true, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS_EQUAL, false, 0, 0, {}, {}}
            }
        };

        for(auto& dssMapEntry : DepthStencilStateMap)
        {
            if(dssMapEntry.Type == eDepthStencilState::DepthOffStencilOff)
            {
                continue;
            }
            result = mDevice->CreateDepthStencilState(&dssMapEntry.Desc, &mDepthStencilStates[static_cast<uint8_t>(dssMapEntry.Type)]);
            if (FAILED(result))
            {
                return E_FAIL;
            }
        }

        if(!createRasterState())
        {
            return E_FAIL;
        }

        return S_OK;
    }

    bool Renderer::initialize(HWND handleWindow, int16_t width, int16_t height, int16_t frameRate)
    {
        mWindowWidth = width;
        mWindowHeight = height;

        DXGI_SWAP_CHAIN_DESC swapDesc;
        ZeroMemory(&swapDesc, sizeof(swapDesc));
        swapDesc.BufferCount = 1;
        swapDesc.BufferDesc.Width = width;
        swapDesc.BufferDesc.Height = height;
        swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapDesc.BufferDesc.RefreshRate.Numerator = frameRate;
        swapDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDesc.OutputWindow = handleWindow;
        swapDesc.SampleDesc.Count = 1;
        swapDesc.SampleDesc.Quality = 0;
        swapDesc.Windowed = TRUE;

        if (FAILED(CreateDeviceAndSetup(swapDesc, width, height, true)))
        {
            ASSERT(false, "모델데이터 초기화 실패 SetupGeometry");
            return false;
        }

        // set default resources

        if (!createGBufferRenderTargets())
        {
            ASSERT(false, "FAIL : createGBufferRenderTargets");
            return false;
        }

        if (FAILED(CreateShadowRenderTarget()))
        {
            ASSERT(false, "FAIL : CreateShadowRenderTarget");
            return false;
        }

        if (FAILED(createSamplerState()))
        {
            ASSERT(false, "FAIL : createSamplerState");
            return false;
        }

        if (!createPresetBlendStates())
        {
            ASSERT(false, "FAIL : create preset blendStates");
            return false;
        }

        constexpr PrimitiveTopologyMap TopologyMap[] =
        {
            {ePrimitiveTopology::Triangles, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST},
            {ePrimitiveTopology::TriangleStrip, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP},
            {ePrimitiveTopology::Lines, D3D11_PRIMITIVE_TOPOLOGY_LINELIST},
        };
        static_assert((sizeof(TopologyMap) / sizeof(PrimitiveTopologyMap)) == static_cast<uint8_t>(ePrimitiveTopology::TopologyCount));
#if defined(_DEBUG)
        for (int32_t topology = 0; topology < static_cast<uint8_t>(ePrimitiveTopology::TopologyCount); ++topology)
        {
            ASSERT(TopologyMap[topology].UserType == static_cast<ePrimitiveTopology>(topology), "열거값과 Map 순서가 일치하지 않음. indexInMap(%d): Map.UserType(%d) != enum(%d))", topology, static_cast<uint8_t>(TopologyMap[topology].UserType), static_cast<uint8_t>(topology));
        }
#endif
        (void)memcpy(mPrimitiveTopologies, TopologyMap, sizeof(TopologyMap));


        const RenderTargetBindDesc BindDescMap[] =
        {
            {eRenderPass::Main,
             5,
                {
                    mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::GBufferColor)],
                    mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::GBufferNormal)],
                    mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::GBufferPosition)],
                    mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::GBufferSpecular)],
                    mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::GBufferAmbient)],
                    nullptr, nullptr, nullptr} ,
                mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Default)]
            },
            {eRenderPass::Shadow,
                1,
                {mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::Shadow)], nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr} ,
                mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Shadow)]
            },
            {eRenderPass::UI,
                1,
                {mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::Default)], nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr} ,
                mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Default)]
            },
            {eRenderPass::Deferred,
                1,
                {mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::Default)], nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr} ,
                nullptr
            },
        };
        static_assert(std::size(BindDescMap) == static_cast<uint8_t>(eRenderPass::PassCount), "RenderPass와 BindDescMap의 짝이 맞아야 합니다.");
#ifdef _DEBUG
        for(uint8_t renderPass = 0; renderPass < static_cast<uint8_t>(eRenderPass::PassCount); ++renderPass)
        {
            if(static_cast<eRenderPass>(renderPass) != BindDescMap[renderPass].RenderPass)
            {
                ASSERT(false, "eRenderPass 멤버 순서와 BindDescMap 요소 순서는 일치해야 합니다.");
            }
        }
#endif

        (void)memcpy(mRenderTargetBindDescMap, BindDescMap, sizeof(BindDescMap));
        return true;
    }

    bool Renderer::createPresetBlendStates()
    {
        constexpr BlendStatePreset BlendStatePresetMap[] =
        {
            {eBlendState::Opaque, D3D11_BLEND_ONE, D3D11_BLEND_ZERO},
            {eBlendState::AlphaBlend, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA},
        };

        for(const BlendStatePreset& preset : BlendStatePresetMap)
        {
            D3D11_BLEND_DESC desc = {};
            desc.RenderTarget[0].BlendEnable = preset.Type != eBlendState::Opaque;
            desc.RenderTarget[0].SrcBlend = static_cast<D3D11_BLEND>(preset.SrcBlend);
            desc.RenderTarget[0].DestBlend = static_cast<D3D11_BLEND>(preset.DestBlend);
            desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

            if (FAILED(mDevice->CreateBlendState(&desc, &mBlendStates[static_cast<int8_t>(preset.Type)])))
            {
                ASSERT(false, "failed to create BlendState. Type(%d)", static_cast<int8_t>(preset.Type));
                return false;
            }
        }
        return true;
    }

    bool Renderer::createGBufferRenderTargets()
    {
        bool bSuccess = false;

        constexpr DXGI_FORMAT TexFormatByBufferType[] =
        {
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_R32G32B32A32_FLOAT, // GBufferColor
            DXGI_FORMAT_R16G16B16A16_FLOAT, // GBufferNormal
            DXGI_FORMAT_R32G32B32A32_FLOAT, // GBufferPosition
            DXGI_FORMAT_R16G16B16A16_FLOAT, // GBufferSpecular
            DXGI_FORMAT_R16G16B16A16_FLOAT, // GBufferAmbient
        };
        static_assert(std::size(TexFormatByBufferType) == static_cast<size_t>(eRenderTarget::RenderTargetCount), "Format 테이블은 RenderTarget 수와 일치해야 합니다.");

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = mWindowWidth;
        texDesc.Height = mWindowHeight;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.SampleDesc.Count = 1;                      // 멀티샘플링 수
        texDesc.SampleDesc.Quality = 0;                    // 멀티샘플링 퀼리티
        texDesc.Usage = D3D11_USAGE_DEFAULT;               // 디폴트로 사용
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;                        // cpu 액세스 여부
        texDesc.MiscFlags = 0;
        for (uint8_t renderTarget = 0; renderTarget < static_cast<uint8_t>(eRenderTarget::RenderTargetCount); ++renderTarget)
        {
            if (TexFormatByBufferType[renderTarget] == DXGI_FORMAT_UNKNOWN)
            {
                continue;
            }

            texDesc.Format = TexFormatByBufferType[renderTarget];
            ID3D11Texture2D* tex = nullptr;
            const bool bTexCreated = SUCCEEDED(CreateTexture2D(texDesc, &tex, "Renderer::GBufferTex"));

            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = TexFormatByBufferType[renderTarget];
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;
            const bool bRenderTargetCreated= SUCCEEDED(CreateRenderTargetView(tex, &rtvDesc, &mRenderTargetViewList[renderTarget]));
            SET_PRIVATE_DATA(mRenderTargetViewList[renderTarget], "eRenderTarget::GBufferRT");


            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = TexFormatByBufferType[renderTarget];
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            const bool bShaderResourceCreated = SUCCEEDED(mDevice->CreateShaderResourceView(tex, &srvDesc, &mRenderTargetSRVs[renderTarget]));
            SET_PRIVATE_DATA(mRenderTargetSRVs[renderTarget], "eRenderTarget::GBufferSRV");

            bSuccess = bTexCreated && bRenderTargetCreated && bShaderResourceCreated;
            SAFETY_RELEASE(tex);
            if (bSuccess == false)
            {
                ASSERT(false, "GBuffer 생성 실패. RenderTarget(%u) state TexCreated(%d), RtCreated(%d), SrvCreated(%d)", renderTarget, static_cast<int32_t>(bTexCreated), static_cast<int32_t>(bRenderTargetCreated), static_cast<int32_t>(bShaderResourceCreated));
                SAFETY_RELEASE(mRenderTargetViewList[renderTarget]);
                SAFETY_RELEASE(mRenderTargetSRVs[renderTarget]);
                break;
            }
        }
        return bSuccess;
    }

    HRESULT Renderer::CreateRenderTargetView(ID3D11Texture2D* const texture, D3D11_RENDER_TARGET_VIEW_DESC* const desc,
                                             ID3D11RenderTargetView** outRtv, const char* const debugTag) const
    {
        ASSERT(texture != nullptr, "texture) do not pass nullptr");
        ASSERT(outRtv != nullptr, "outRtv) do not pass nullptr.");
        ASSERT((*outRtv) == nullptr, "outRtv)pRtv is already initialized.");

        const HRESULT result = mDevice->CreateRenderTargetView(texture, desc, outRtv);
        if(FAILED(result))
        {
            ASSERT(false, "failed to create RenderTargetView: RenderTargetView 생성 실패");
            ASSERT(false, "%hs", debugTag);
        }

        return result;
    }

    HRESULT Renderer::CreateDepthStencilView(ID3D11Texture2D* const texture, D3D11_DEPTH_STENCIL_VIEW_DESC* const desc,
        ID3D11DepthStencilView** outDs, const char* const debugTag) const
    {
        ASSERT(texture != nullptr, "texture) do not pass nullptr");
        ASSERT(outDs != nullptr, "outDs) do not pass nullptr.");
        ASSERT((*outDs) == nullptr, "outDs)pOutDs is already initialized.");

        const HRESULT result = mDevice->CreateDepthStencilView(texture, desc, outDs);
        if (FAILED(result))
        {
            ASSERT(false, "failed to create DepthStencilView: DepthStencilView 생성 실패");
            ASSERT(false, "%hs", debugTag);
        }

        return result;
    }

    void Renderer::BindRenderTargetByRenderPass(eRenderPass pass)
    {
        ASSERT(pass != eRenderPass::PassCount, "올바르지 않은 RenderPass Type. pass(%d)", static_cast<uint8_t>(pass));

        const uint8_t passIndex = static_cast<uint8_t>(pass);
        mDeviceContext->OMSetRenderTargets(mRenderTargetBindDescMap[passIndex].ViewCount, mRenderTargetBindDescMap[passIndex].RenderTargetViews, mRenderTargetBindDescMap[passIndex].DepthStencilViews);
    }

    void Renderer::BindInputLayoutTo(eVertexFormat type) const
    {
        ID3D11InputLayout* const intputLayout = mShaderManager->GetInputLayoutByType(type);
        mDeviceContext->IASetInputLayout(intputLayout);
    }

    void Renderer::BindShaderTo(eShader type) const
    {
        ID3D11VertexShader* vs = nullptr;
        ID3D11PixelShader* ps = nullptr;
        mShaderManager->GetShadersByType(type, &vs, &ps);
        mDeviceContext->VSSetShader(vs, nullptr, 0U);
        mDeviceContext->PSSetShader(ps, nullptr, 0U);
    }

    void Renderer::Draw(uint32_t vertexCount, uint32_t startVertexLocation) const
    {
        mDeviceContext->Draw(vertexCount, startVertexLocation);
    }

    void Renderer::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation) const
    {
        mDeviceContext->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
    }

    void Renderer::SetViewport(bool bFullScreen) const
    {
        if (bFullScreen)
        {
            mDeviceContext->RSSetViewports(1U, &mViewportFull);
        }
        else
        {
            mDeviceContext->RSSetViewports(1U, &mViewportTex);
        }
    }

    HRESULT Renderer::CreateShadowRenderTarget()
    {
        /*
         * 렌더 타겟의 사이즈와 거기에 붙인 텍스쳐들의 사이즈는 동일해야 한다.
         * 따라서 텍스쳐만 갈아끼우며 사용하려면 지정한 사이즈와 동일한 사이즈의 텍스쳐로 사용해야 한다.
         * 
         */
       /* uint32_t texWidth = 2048U;
        uint32_t texHeight = 2048U;*/

        uint32_t texWidth = 4096;
        uint32_t texHeight = 4096U;
        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = texWidth;
        depthDesc.Height = texHeight;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        depthDesc.SampleDesc.Count = 1;                      // 멀티샘플링 수
        depthDesc.SampleDesc.Quality = 0;                    // 멀티샘플링 퀼리티
        depthDesc.Usage = D3D11_USAGE_DEFAULT;               // 디폴트로 사용
        depthDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
        depthDesc.CPUAccessFlags = 0;                        // cpu 액세스 여부
        depthDesc.MiscFlags = 0;
        ID3D11Texture2D* texColor = nullptr;
        CreateTexture2D(depthDesc, &texColor, "Renderer::texColor"); // mTexShadow


        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = depthDesc.Format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        HRESULT result = CreateRenderTargetView(texColor, &rtvDesc, &mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::Shadow)]); // mShadowRtv
        if (FAILED(result))
        {
            return E_FAIL;
        }
        SET_PRIVATE_DATA(mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::Shadow)], "eRenderTarget::Shadow");
        SAFETY_RELEASE(texColor);

        mViewportTex.Width    = static_cast<float>(texWidth);
        mViewportTex.Height   = static_cast<float>(texHeight);
        mViewportTex.MinDepth = 0.0f;
        mViewportTex.MaxDepth = 1.0f;
        mViewportTex.TopLeftX = 0;
        mViewportTex.TopLeftY = 0;
        // 렌더링될 영역을 지정. s가 붙으니까 여러개 지정가능.(모델링 프로그램을 생각)
        // rasterizer stage
        mDeviceContext->RSSetViewports(1, &mViewportTex); // mViewportForTex 
     
        depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        ID3D11Texture2D* texShadow = nullptr;
        CreateTexture2D(depthDesc, &texShadow, "Renderer::texShadow"); // mTexShadow

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
        depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Texture2D.MipSlice = 0;

        result = CreateDepthStencilView(texShadow, &depthStencilViewDesc, &mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Shadow)]); // mShadowDsv
        if (FAILED(result))
        {
            ASSERT(false, "mShadowDsv 생성 실패");
            return E_FAIL;
        }
        SET_PRIVATE_DATA(mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Shadow)], "eRenderTarget::Shadow");

        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Shadow)].RenderTargetIndex = 1U;
        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Shadow)].DepthStencilIndex = 1U;
        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Shadow)].NumViews = 1U;


        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        ZeroMemory(&desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 1;
        desc.Texture2D.MostDetailedMip = 0;
        desc.Format = DXGI_FORMAT_R32_FLOAT;
        result = mDevice->CreateShaderResourceView(texShadow, &desc, &mRenderTargetSRVs[static_cast<uint8_t>(eRenderTarget::Shadow)]);
        if (FAILED(result))
        {
            ASSERT(false, "Failed to create texShadow");
        }

        SAFETY_RELEASE(texShadow);

        return S_OK;
    }

    HRESULT Renderer::CreateTexture2D(D3D11_TEXTURE2D_DESC& desc, ID3D11Texture2D** outTex, const char* tag) const
    {
        ASSERT(*outTex == nullptr, "pass nullptr before create texture.");
        if(!tag)
        {
            tag = "Renderer::UNKNOWN_TEXTURE";
        }

        HRESULT result = mDevice->CreateTexture2D(&desc, nullptr, &(*outTex));
        if (FAILED(result))
        {
            ASSERT(false, "dRenderer::MyCreateTexture ) 텍스처 생성 실패");
            result = E_FAIL;
        }
        SET_PRIVATE_DATA((*outTex), tag);
        return result;
    }

    void Renderer::BindVertexBuffer(uint32_t stride) const
    {
        constexpr uint32_t offset = 0;
        ID3D11Buffer* const vertexBuffer = mBufferManager->GetVertexBuffer(static_cast<int16_t>(stride));
        mDeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    }

    void Renderer::BindIndexBuffer() const
    {
        uint32_t offset = 0;
        const int16_t stride = mBufferManager->GetIndexStrideSize();
        const DXGI_FORMAT format = mBufferManager->GetIndexFormat();
        ID3D11Buffer* const indexBuffer = mBufferManager->GetIndexBuffer(stride);
        mDeviceContext->IASetIndexBuffer(indexBuffer, format, offset);
    }

    void Renderer::BindVertexBufferDynamic(uint32_t stride) const
    {
        uint32_t offset = 0;
        ID3D11Buffer* const vertexBuffer = mBufferManager->GetVertexBufferDynamic(static_cast<int16_t>(stride));
        mDeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    }

    void Renderer::BindIndexBufferDynamic() const
    {
        uint32_t offset = 0;
        const int16_t stride = mBufferManager->GetIndexStrideSize();
        const DXGI_FORMAT format = mBufferManager->GetIndexFormat();
        ID3D11Buffer* const indexBuffer = mBufferManager->GetIndexBufferDynamic(stride);
        mDeviceContext->IASetIndexBuffer(indexBuffer, format, offset);
    }

    void Renderer::BindSamplerToPsByType(uint32_t slot, eSamplerType type) const
    {
        mDeviceContext->PSSetSamplers(slot, 1, &mSamplerState[static_cast<int32_t>(type)]);
    }

    void Renderer::BindBlendStateByType(eBlendState type) const
    {
        ASSERT(type != eBlendState::StateCount, "올바르지 않은 type. type(%d)", static_cast<uint8_t>(type));
        mDeviceContext->OMSetBlendState(mBlendStates[static_cast<uint8_t>(type)], nullptr, 0xffff'ffff);
    }

    void Renderer::BindTextureToPs(uint32_t slot, HashID textureHash) const
    {
        ASSERT(textureHash > 0, "invalid hash. hash(%u)", textureHash);
        ID3D11ShaderResourceView* srv = mTextureManager->GetTextureByHash(textureHash);
        if(srv)
        {
            mDeviceContext->PSSetShaderResources(slot, 1, &srv);
        }
    }

    void Renderer::UnbindTexturePs(uint32_t slot) const
    {
        ID3D11ShaderResourceView* unbindSRV = nullptr;
        mDeviceContext->PSSetShaderResources(slot, 1, &unbindSRV);
    }

    void Renderer::BindPrimitiveTopologyByType(ePrimitiveTopology topology) const
    {
        const PrimitiveTopologyMap topologyElement = mPrimitiveTopologies[static_cast<uint8_t>(topology)];
        mDeviceContext->IASetPrimitiveTopology(topologyElement.ApiType);
    }

    void Renderer::BindRasterStateByType(eRasterType type) const
    {
        mDeviceContext->RSSetState(mRasterStates[static_cast<uint32_t>(type)]);
    }

    void Renderer::BindDepthStencilState(eDepthStencilState type) const
    {
        ASSERT(type != eDepthStencilState::StateCount, "올바르지 않은 eDepthStencilState 유형. 쓰지 않으려면 DepthOff를 지정해야 합니다. type(%d)", static_cast<int8_t>(type))
        mDeviceContext->OMSetDepthStencilState(mDepthStencilStates[static_cast<uint8_t>(type)], 0);
    }

    void Renderer::ClearAllScreenAndDepth() const
    {
        constexpr float CLEAR_COLOR[] = { 0.4f, 0.6f, 1.0f, 1.0f };
        for (uint8_t renderTarget = 0; renderTarget < static_cast<uint8_t>(renderer::eRenderTarget::RenderTargetCount); ++renderTarget)
        {
            mDeviceContext->ClearRenderTargetView(mRenderTargetViewList[renderTarget], CLEAR_COLOR);
            if(mDepthStencilViewList[renderTarget])
            {
                mDeviceContext->ClearDepthStencilView(mDepthStencilViewList[renderTarget], D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
            }
        }
    }

    void Renderer::ClearDepthBuffer() const
    {
        mDeviceContext->ClearDepthStencilView(mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Default)], D3D11_CLEAR_DEPTH, 1.0f, 0);
    }

    void Renderer::Present() const
    {
        mSwapChain->Present(0, 0);
    }

    bool Renderer::CheckDeviceLost(bool& outIsReInitialize) const
    {
        bool bTerminateProgram = false;
        switch (mDevice->GetDeviceRemovedReason())
        {
        case DXGI_ERROR_DEVICE_HUNG:
        case DXGI_ERROR_DEVICE_RESET:
        {
            outIsReInitialize = true;
            break;
        }
        case S_OK:
        {
            outIsReInitialize = false;
            break;
        }
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
        case DXGI_ERROR_INVALID_CALL:
        {
            bTerminateProgram = true;
            outIsReInitialize = false;
            break;
        }
        default:
        {
            // MEMO: 다른 에러는 일단 돌게 만드는데, 계속 써보면서 다른 에러처리를 추가하도록 함
            outIsReInitialize = false;
            break;
        }
        }
        return bTerminateProgram;
    }

    void Renderer::Cleanup()
    {

        for (uint32_t i = 0; i < static_cast<uint32_t>(eRasterType::RasterCount); ++i)
        {
            SAFETY_RELEASE(mRasterStates[i]);
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(eSamplerType::SamplerCount); ++i)
        {
            SAFETY_RELEASE(mSamplerState[i]);
        }

        for (uint32_t i = 0; i < static_cast<uint8_t>(eBlendState::StateCount); ++i)
        {
            SAFETY_RELEASE(mBlendStates[i]);
        }

        for (uint32_t i = 0; i < static_cast<uint8_t>(eRenderTarget::RenderTargetCount); ++i)
        {
            SAFETY_RELEASE(mRenderTargetViewList[i]);
            SAFETY_RELEASE(mDepthStencilViewList[i]);
            SAFETY_RELEASE(mRenderTargetSRVs[i]);
        }

        for (uint32_t state = 0; state < static_cast<uint8_t>(eDepthStencilState::StateCount); ++state)
        {
            SAFETY_RELEASE(mDepthStencilStates[state]);
        }
        SAFETY_RELEASE(mSwapChain);
        SAFETY_RELEASE(mDeviceContext);
        SAFETY_RELEASE(mDevice);
    }

    ULONG Renderer::AddRef()
    {
        return InterlockedIncrement(&mRefCount);
    }

    ULONG Renderer::Release()
    {
        const uint32_t ref = InterlockedDecrement(&mRefCount);
        if(ref <= 0)
        {
            delete this;
        }
        return ref;
    }

    HRESULT Renderer::QueryInterface(const IID& riid, void** ppvObject)
    {
        ASSERT(false, "not implements");
        return E_FAIL;
    }

    void Renderer::BindCbToVsByType(uint32_t slot, uint32_t numBuffer, eCbType type) const
    {
        ID3D11Buffer* const cBuffer = mShaderManager->GetConstantBufferByType(type);
        mDeviceContext->VSSetConstantBuffers(slot, numBuffer, &cBuffer);
    }


    void Renderer::BindCbToPs(uint32_t slot, uint32_t numBuffer, eCbType type) const
    {
        ID3D11Buffer* const cBuffer = mShaderManager->GetConstantBufferByType(type);
        mDeviceContext->PSSetConstantBuffers(slot, numBuffer, &cBuffer);
    }
}
