#include "Renderer.h"
#include <algorithm>
#include <fstream>
#include "../Util/Macro.h"
#include "../Util/Util.h"
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
        , mGraphicsQueueIndex(UINT_MAX)
        , mDevice(nullptr)
        , mQueue(nullptr)
        , mSurface(nullptr)
        , mSurfaceFormat()
        , mSwapChain(nullptr)
        , mSwapChainExtent()
        , mFrames{}
        , mFrameCount(0)
        , mCurImageIndex(0)
        , mSimplePipeline(nullptr)
        , mVertexBuffer()
        , mIndexBuffer()
        , mUbPerFrames{}
        , mUbPerObjects{}
        , mShaderDescriptors{}
        , mbStagingEnabled(false)
    {
    }

    VkRenderer::~VkRenderer()
    {
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
            vkDestroySampler(mDevice, samplerIt.second, nullptr);
        }

        for(const auto& shaderDescriptor : mShaderDescriptors)
        {
            vkDestroyDescriptorPool(mDevice, shaderDescriptor.Pool, nullptr);
        }

        for(const auto& pipelineIt : mPipelines)
        {
            vkDestroyPipeline(mDevice, pipelineIt.second, nullptr);
        }
        mPipelines.clear();

        for(const auto& shaderLayoutIt : mShaderLayouts)
        {
            vkDestroyPipelineLayout(mDevice, shaderLayoutIt.second.PipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(mDevice, shaderLayoutIt.second.SetLayout, nullptr);
        }
        mShaderLayouts.clear();

        for(const auto& shaderInfo : mShaderInfos)
        {
            vkDestroyShaderModule(mDevice, shaderInfo.second.ShaderModule, nullptr);
        }
        mShaderInfos.clear();

        for (const auto& uniformBuffer : mUbPerFrames)
        {
            vkFreeMemory(mDevice, uniformBuffer.Memory, nullptr);
            vkDestroyBuffer(mDevice, uniformBuffer.Buffer, nullptr);
        }
        for (const auto& uniformBuffer : mUbPerObjects)
        {
            vkFreeMemory(mDevice, uniformBuffer.Memory, nullptr);
            vkDestroyBuffer(mDevice, uniformBuffer.Buffer, nullptr);
        }
        vkFreeMemory(mDevice, mVertexBuffer.Memory, nullptr);
        vkDestroyBuffer(mDevice, mVertexBuffer.Buffer, nullptr);
        mVertexBuffer.SubResources.clear();

        vkFreeMemory(mDevice, mIndexBuffer.Memory, nullptr);
        vkDestroyBuffer(mDevice, mIndexBuffer.Buffer, nullptr);
        mIndexBuffer.SubResources.clear();

        for(const auto& frame : mFrames)
        {
            vkDestroyCommandPool(mDevice, frame.Pool, nullptr);
            vkDestroySemaphore(mDevice, frame.ImageReadySemaphore, nullptr);
            vkDestroyFence(mDevice, frame.DrawFinishedFence, nullptr);
        }

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

        if (createImageViews() == false)
        {
            return false;
        }

        if(createFrame() == false)
        {
            return false;
        }

        if(createSyncObjects() == false)
        {
            return false;
        }

        if(createSamplers() == false)
        {
            return false;
        }

        return true;
    }

    void VkRenderer::PrepareObjects()
    {
        CreateBuffer(771336 * 2, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), mGraphicsQueueIndex, false, mVertexBuffer);
        CreateBuffer(189792 * 2, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), mGraphicsQueueIndex, false, mIndexBuffer);
        for (auto& mUniformBuffer : mUbPerFrames)
        {
            CreateBuffer(1024, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), mGraphicsQueueIndex, false, mUniformBuffer);
        }
        for (auto& mUniformBuffer : mUbPerObjects)
        {
            CreateBuffer(1024, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), mGraphicsQueueIndex, false, mUniformBuffer);
        }
        const int8_t* const modelFilePath = reinterpret_cast<int8_t*>("/AssetData/textures/Unagi_Body_D.png");
        HashID texHash;
        CreateTexture(modelFilePath, texHash, false);
    }

    void VkRenderer::UpdateScene(std::vector<std::pair<renderer::eCbType, void*>>& uniforms)
    {
        const uint32_t frameIndex = getCurrentFrameIndex();
        for(const auto& uniform : uniforms)
        {
            if(uniform.first == eCbType::CbViewProj)
            {
                UpdateUniformData(uniform.second, sizeof(CbViewProj), mUbPerFrames[frameIndex]);
            }
            else
            {
                UpdateUniformData(uniform.second, sizeof(CbWorld) , mUbPerObjects[frameIndex]);
            }
        }
        UpdateDescriptorSet(eShader::Simple);
    }

    void VkRenderer::RenderScene()
    {
        // Scene Update
        const PerFrame frame = getCurrentFrameResource();
        const uint32_t frameIndex = getCurrentFrameIndex();
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = mSwapChainExtent.width;
        viewport.height = mSwapChainExtent.height;
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

        vkCmdSetViewport(frame.Buffer, 0, 1, &viewport);

        VkRect2D scissor{ {0, 0}, mSwapChainExtent };
        vkCmdSetScissor(frame.Buffer, 0, 1, &scissor);


        vkCmdBindPipeline(frame.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSimplePipeline);

        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(frame.Buffer, 0, 1, &mVertexBuffer.Buffer, &offset);

        const VkIndexType indexType = VK_INDEX_TYPE_UINT32;
        vkCmdBindIndexBuffer(frame.Buffer, mIndexBuffer.Buffer, 0, indexType);

        const auto& shaderLayoutIt = mShaderLayouts.find(eShader::Simple);

        vkCmdBindDescriptorSets(frame.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderLayoutIt->second.PipelineLayout, 0, 1, &mShaderDescriptors[static_cast<uint8_t>(eShader::Simple)].Set[frameIndex], 0, nullptr);
    }

    void VkRenderer::DrawIndexed(uint32_t indexCount, uint32_t vertexOffset, uint32_t indexOffset)
    {
        const PerFrame frame = getCurrentFrameResource();
        vkCmdDrawIndexed(frame.Buffer, indexCount, 1, indexOffset, vertexOffset, 0);
    }

    void VkRenderer::WaitForLastFrame() const
    {
        const PerFrame curFrame = getCurrentFrameResource();
        // wait 1sec = 1'000'000'000
        const VkResult result = vkWaitForFences(mDevice, 1, &curFrame.DrawFinishedFence, VK_TRUE, 1'000'000'000); 
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "Fence Wait LastFrame 실패 resultCode(%d)", result);
        }
        (void)vkResetFences(mDevice, 1, &curFrame.DrawFinishedFence);
    }

    void VkRenderer::BeginRender()
    {
        const PerFrame curFrame = getCurrentFrameResource();

        mCurImageIndex = acquireSwapChainImage(curFrame);

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        // MEMO: 암묵적 초기화
        const VkResult result = vkBeginCommandBuffer(curFrame.Buffer, &commandBufferBeginInfo);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "vkBeginCommandBuffer 실패 resultCode(%d)", result);
        }

        transitionImageLayout(curFrame.Buffer, mCurImageIndex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT , VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

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
        vkCmdBeginRendering(curFrame.Buffer, &renderingInfo);
    }

    void VkRenderer::EndRender()
    {
        const PerFrame curFrame = getCurrentFrameResource();

        vkCmdEndRendering(curFrame.Buffer);

        transitionImageLayout(curFrame.Buffer, mCurImageIndex, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
        vkEndCommandBuffer(curFrame.Buffer);
    }

    void VkRenderer::SubmitCommands() const
    {
        const PerFrame curFrame = getCurrentFrameResource();

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
        commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferSubmitInfo.commandBuffer = curFrame.Buffer;

        VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo{};
        signalSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalSemaphoreSubmitInfo.semaphore = mRenderFinishedSemaphores[mCurImageIndex];
        signalSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
        signalSemaphoreSubmitInfo.value = 1;

        VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo{};
        waitSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        waitSemaphoreSubmitInfo.semaphore = curFrame.ImageReadySemaphore;
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
        vkQueueSubmit2(mQueue, 1, & submitInfo, curFrame.DrawFinishedFence);
    }

    void VkRenderer::Present() const
    {
        const PerFrame curFrame = getCurrentFrameResource();

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pSwapchains = &mSwapChain;
        presentInfo.swapchainCount = 1;
        presentInfo.pImageIndices = &mCurImageIndex;

        presentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[mCurImageIndex];
        presentInfo.waitSemaphoreCount = 1;

        vkQueuePresentKHR(mQueue, &presentInfo);
    }

    void VkRenderer::CountFrame()
    {
        ++mFrameCount;
        mFrameCount = (mFrameCount == UINT64_MAX) ? mFrameCount % sOverlappedFrame : mFrameCount;
    }

    VkPipeline VkRenderer::GetPipeline(HashID hash)
    {
        VkPipeline pipeline{};
        const auto& pipelineIt = mPipelines.find(hash);
        if(pipelineIt == mPipelines.end())
        {
            ASSERT(false, "존재하지 않는 pipeline의 hash이거나 생성되지 않은 hash를 찾으려고 합니다. hash(%u)", hash);
        }
        else
        {
            pipeline = pipelineIt->second;
        }

        return pipeline;
    }

    void VkRenderer::SetSimplePipeline(HashID hash)
    {
        const auto& pipelineIt = mPipelines.find(hash);
        mSimplePipeline = pipelineIt->second;
    }

    bool VkRenderer::CreateShaderStage(const char* const path, bool bMakeAsPixelShader, HashID& outShaderStageHash)
    {
        ASSERT(path, "path는 유효해야합니다. path(%s)", path);

        HashID hash = util::GetDjb2Hash(reinterpret_cast<const int8_t*>(path));
        if(mShaderInfos.find(hash) != mShaderInfos.end())
        {
            outShaderStageHash = hash;
            return true;
        }
        std::ifstream shaderFile(path, std::ios::ate | std::ios::binary);

        if(shaderFile.is_open() == false)
        {
            ASSERT(false, "셰이더 파일 로드 실패 ");
            return false;
        }

        const uint32_t fileSize = shaderFile.tellg();
        shaderFile.seekg(0, std::ios::beg);

        std::vector<uint8_t> shaderCode(fileSize);

        shaderFile.read(reinterpret_cast<char*>(shaderCode.data()), fileSize);

        ShaderStage shaderInfo{};
        // make shaderModule
        
        VkShaderModuleCreateInfo shaderModuleCreateInfo{};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = fileSize;
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

        const VkResult result = vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &shaderInfo.ShaderModule);
        if(result != VK_SUCCESS)
        {
            ASSERT(false, "vkCreateShaderModule 실패 resultCode(%d)", result);
            return false;
        }
        // make shaderStgae

        shaderInfo.PipelineShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderInfo.PipelineShaderStage.stage = bMakeAsPixelShader ? VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT : VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
        shaderInfo.PipelineShaderStage.module = shaderInfo.ShaderModule;
        shaderInfo.PipelineShaderStage.pName = "main";

        mShaderInfos.insert({hash, std::move(shaderInfo)});
        outShaderStageHash = hash;
        return true;
    }

    bool VkRenderer::CreateShaderLayout(eShader shader)
    {
        const auto& shaderLayoutIt = mShaderLayouts.find(shader);
        if (shaderLayoutIt == mShaderLayouts.end())
        {
            const std::unordered_map<eShader, std::vector<VkDescriptorSetLayoutBinding>> DescriptorLayoutBindingTable =
            {
                {
                    eShader::BasicWithShadow,
                    {
                        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {7, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {13, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {15, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    }
                },
                {
                    eShader::Shadow,
                    {
                        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                    }
                },
                {
                    eShader::Texture,
                    {
                        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {7, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {13, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    }
                },
                {
                    eShader::Color,
                    {
                        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {15, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    }
                },
                {
                    eShader::DebugColor,
                    {
                        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {15, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    }
                },
                {
                    eShader::DebugHUD,
                    {
                        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {7, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {13, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    }
                },
                {
                    eShader::Deferred,
                    {
                       {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {7, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {8, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {9, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {10, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {11, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {12, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {13, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    }
                },
                {
                    eShader::Outline,
                    {
                       {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                    }
                },
                {
                    eShader::Skybox,
                    {
                       {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                        {7, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {13, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                        {15, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                    }
                },
                {
                    eShader::Simple,
                    {
                       {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT , nullptr},
                        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT , nullptr},
                    }
                }
            };

            ASSERT(DescriptorLayoutBindingTable.find(shader) != DescriptorLayoutBindingTable.end(), "유효하지 않은 shader이거나, shader에 대한 DescriptorTable에 추가되지 않았습니다. shader(%u)", static_cast<uint8_t>(shader));

            const auto& descriptorLayoutIt = DescriptorLayoutBindingTable.find(shader);
            ASSERT(descriptorLayoutIt != DescriptorLayoutBindingTable.end(), "shader 타입이 올바르지 않거나, Table에 추가되지 않았습니다");

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.pBindings = descriptorLayoutIt->second.data();
            descriptorSetLayoutCreateInfo.bindingCount = descriptorLayoutIt->second.size();

            VkDescriptorSetLayout setLayout;
            VkResult result = vkCreateDescriptorSetLayout(mDevice, &descriptorSetLayoutCreateInfo, nullptr, &setLayout);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "DescriptorSetLayout 생성 실패.  result(%u)", result);
            }

            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.pSetLayouts = &setLayout;
            pipelineLayoutCreateInfo.setLayoutCount = 1;
            pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

            VkPipelineLayout pipelineLayout;
            result = vkCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "PipelineLayout 생성 실패.  result(%u)", result);
            }

            ShaderLayout shaderLayout{ setLayout, pipelineLayout, {} };
            shaderLayout.DescriptorInfos.reserve(descriptorLayoutIt->second.size());
            for(const auto& bindingInfo : descriptorLayoutIt->second)
            {
                shaderLayout.DescriptorInfos.push_back({bindingInfo.descriptorType, bindingInfo.descriptorCount});
            }

            mShaderLayouts.insert({ shader, std::move(shaderLayout) });
        }
        return true;
    }

    bool VkRenderer::CreateDescriptorSet()
    {
        for (uint8_t shader = 0; shader < static_cast<uint8_t>(eShader::ShaderCount); ++shader)
        {
            const auto& shaderLayoutIt = mShaderLayouts.find(static_cast<eShader>(shader));

            std::vector<VkDescriptorPoolSize> poolSizes;
            poolSizes.reserve(shaderLayoutIt->second.DescriptorInfos.size());
            for(const auto& descriptorInfo : shaderLayoutIt->second.DescriptorInfos)
            {
                poolSizes.push_back({descriptorInfo.Type, descriptorInfo.Count * sOverlappedFrame });
            }

            VkDescriptorPoolCreateInfo poolCreateInfo{};
            poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolCreateInfo.maxSets = sOverlappedFrame;
            poolCreateInfo.pPoolSizes = poolSizes.data();
            poolCreateInfo.poolSizeCount = poolSizes.size();
            poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            
            VkResult result = vkCreateDescriptorPool(mDevice, &poolCreateInfo, nullptr, &mShaderDescriptors[shader].Pool);
            if(result != VK_SUCCESS)
            {
                ASSERT(false, "DescriptorPool 생성 실패.  result(%u)", result);
                return false;
            }
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts(sOverlappedFrame, shaderLayoutIt->second.SetLayout);
            VkDescriptorSetAllocateInfo setAllocateInfo{};
            setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            setAllocateInfo.descriptorPool = mShaderDescriptors[shader].Pool;
            setAllocateInfo.descriptorSetCount = descriptorSetLayouts.size();
            setAllocateInfo.pSetLayouts = descriptorSetLayouts.data();
            result = vkAllocateDescriptorSets(mDevice, &setAllocateInfo, mShaderDescriptors[shader].Set);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "DescriptorSet 생성 실패.  result(%u)", result);
                return false;
            }
        }
        return true;
    }

    bool VkRenderer::UpdateDescriptorSet(eShader shader)
    {
        const uint32_t frameIndex = mFrameCount % sOverlappedFrame;

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = mUbPerFrames[frameIndex].Buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = 0;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = {};
        imageInfo.imageView = 0;
        imageInfo.sampler = {};
        switch(shader)
        {
        case eShader::Outline:
        {
            VkDescriptorBufferInfo bufferInfoPerFrame{};
            bufferInfoPerFrame.buffer = mUbPerFrames[frameIndex].Buffer;
            bufferInfoPerFrame.offset = frameIndex * sizeof(CbViewProj);
            bufferInfoPerFrame.range = sizeof(CbViewProj);

            VkDescriptorBufferInfo bufferInfoPerObject{};
            bufferInfoPerObject.buffer = mUbPerObjects[frameIndex].Buffer;
            bufferInfoPerObject.offset = frameIndex * sizeof(CbWorld);
            bufferInfoPerObject.range = sizeof(CbWorld);

            const std::vector<VkWriteDescriptorSet> WriteDescriptors =
            {
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfoPerObject, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfoPerFrame, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 6, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfoPerFrame, {}},
            };

            vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);

            break;
        }
        case eShader::Skybox:
        {
            //VkDescriptorBufferInfo bufferInfo{};
            //bufferInfo.buffer = mUbPerFrames[frameIndex].Buffer;
            //bufferInfo.offset = 0;
            //bufferInfo.range = 0;

            //VkDescriptorImageInfo imageInfo{};
            //imageInfo.imageLayout = {};
            //imageInfo.imageView = 0;
            //imageInfo.sampler = {};

            //VkWriteDescriptorSet writeDescriptorSet{};
            //writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            //writeDescriptorSet.dstArrayElement = 0; // 배열 업데이트 시작 인덱스;
            //writeDescriptorSet.descriptorCount = 1; // 업데이트 요소 수;
            //writeDescriptorSet.dstSet = mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0];
            //writeDescriptorSet.dstBinding = // 업데이트 할 binding slot;
            //    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            //writeDescriptorSet.pBufferInfo = &bufferInfo;

            const std::vector<VkWriteDescriptorSet> WriteDescriptors =
            {
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 7, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 13, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, {}, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 15, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &imageInfo, {}, {}},
            };

            vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
            break;
        }
        case eShader::Shadow:
        {
            const std::vector<VkWriteDescriptorSet> WriteDescriptors =
            {
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 2, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
            };

            vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
            break;
        }
        case eShader::BasicWithShadow:
        {
            const std::vector<VkWriteDescriptorSet> WriteDescriptors =
            {
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 2, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 3, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 4, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},

                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 7, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 13, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, {}, {}},

                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 15, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &imageInfo, {}, {}},
            };

            vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
            break;
        }
        case eShader::Texture:
        {
            const std::vector<VkWriteDescriptorSet> WriteDescriptors =
            {
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},

                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 7, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 13, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, {}, {}},
            };

            vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
            break;
        }
        case eShader::Color:
        {
            const std::vector<VkWriteDescriptorSet> WriteDescriptors =
            {
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},

                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 15, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &imageInfo, {}, {}},
            };

            vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
            break;
        }
        case eShader::DebugHUD:
        {
            const std::vector<VkWriteDescriptorSet> WriteDescriptors =
            {
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 5, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 7, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 13, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, {}, {}},
            };

            vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
            break;
        }
        case eShader::Deferred:
        {
            const std::vector<VkWriteDescriptorSet> WriteDescriptors =
            {
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 2, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 3, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 4, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},

                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 7, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 8, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 9, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 10, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 11, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 12, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, {}, {}},

                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 13, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, {}, {}},
            };

            vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
            break;
        }
        case eShader::DebugColor:
        {
            const std::vector<VkWriteDescriptorSet> WriteDescriptors =
            {
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfo, {}},

                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[0], 15, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &imageInfo, {}, {}},
            };

            vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
            break;
        }
        case eShader::Simple:
        {
            VkDescriptorBufferInfo bufferInfoPerFrame{};
            bufferInfoPerFrame.buffer = mUbPerFrames[frameIndex].Buffer;
            bufferInfoPerFrame.offset = 0;
            bufferInfoPerFrame.range = sizeof(CbViewProj);

            VkDescriptorBufferInfo bufferInfoPerObject{};
            bufferInfoPerObject.buffer = mUbPerObjects[frameIndex].Buffer;
            bufferInfoPerObject.offset = 0;
            bufferInfoPerObject.range = sizeof(CbWorld);

            const std::vector<VkWriteDescriptorSet> WriteDescriptors =
            {
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[frameIndex], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfoPerObject, {}},
                {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, {}, mShaderDescriptors[static_cast<uint8_t>(shader)].Set[frameIndex], 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {}, &bufferInfoPerFrame, {}},
            };

            vkUpdateDescriptorSets(mDevice, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
            break;
        }
        default:
            ASSERT(false, "올바르지 않은 타입이 전달되었습니다. type(%u)", static_cast<uint8_t>(shader));
            break;
        }
        return true;
    }

    void VkRenderer::UpdateUniformData(void* const pData, uint32_t sizeInBytes, BufferInfo uniformBuffer)
    {
        uploadBuffer(pData, sizeInBytes, 0, uniformBuffer.Memory);
    }

    bool VkRenderer::TryCreateGraphicsPipeline(HashID& outPipelineHash, eShader shader, HashID VsStageHash, HashID PsStageHash, eVertexFormat vertexFormat, ePrimitiveTopology topology, eRasterType rasterType, eBlendState blendState, eDepthStencilState depthStencilState, eRenderPass renderPass)
    {
        // TODO: hash 조합하기
        const HashID hash = MakePipelineHash(shader, vertexFormat, topology, rasterType, blendState, depthStencilState, renderPass);
        bool result = true;
        const auto& pipelineIt = mPipelines.find(hash);
        if(pipelineIt == mPipelines.end())
        {
            VkPipeline newPipeline{};
            result = createGraphicsPipeline(shader, VsStageHash, PsStageHash, vertexFormat, topology, rasterType, blendState, depthStencilState, renderPass, newPipeline);
            if(result)
            {
                mPipelines.insert({ hash, newPipeline });
                outPipelineHash = hash;
            }
        }
        else
        {
            outPipelineHash = pipelineIt->first;
        }

        return result;
    }
    template<typename  T>
    constexpr uint64_t GetBitCount(T type)
    {
        int32_t bit = 1;
        uint64_t count = 0;
        while (bit <= static_cast<uint32_t>(type))
        {
            bit <<= 1;
            ++count;
        }
        return count;
    }
    HashID VkRenderer::MakePipelineHash(eShader shader, eVertexFormat vertexFormat, ePrimitiveTopology topology, eRasterType rasterType, eBlendState blendState, eDepthStencilState depthStencilState, eRenderPass renderPass)
    {
        struct PipelineStateBitField
        {
            HashID Value;
            HashID BitWidth;
        };

        const PipelineStateBitField PipelineBitLayout[] =
        {
            {static_cast<HashID>(shader), GetBitCount(eShader::ShaderCount)},
            {static_cast<HashID>(vertexFormat), GetBitCount(eVertexFormat::FormatCount)},
            {static_cast<HashID>(topology), GetBitCount(ePrimitiveTopology::TopologyCount)},
            {static_cast<HashID>(rasterType), GetBitCount(eRasterType::RasterCount)},
            {static_cast<HashID>(blendState), GetBitCount(eBlendState::StateCount)},
            {static_cast<HashID>(depthStencilState), GetBitCount(eDepthStencilState::StateCount)},
            {static_cast<HashID>(renderPass), GetBitCount(eRenderPass::PassCount)},
        };


#ifdef _DEBUG
        // MEMO: 사용된 Bit들을 전부 합했을 때 자료형을 넘는지 검사
        uint32_t validBitCount = 0;
        for (auto& element : PipelineBitLayout)
        {
            validBitCount += element.BitWidth;
        }
        ASSERT(validBitCount <= 32, "hash가 표현할 수 있는 비트 수를 넘었습니다. 계산된 비트 사용량(%llu)", validBitCount);
#endif

        HashID bitShiftCursor = 32;
        HashID pipelineHash = 0;
        for (auto& element : PipelineBitLayout)
        {
            bitShiftCursor -= element.BitWidth;
            pipelineHash |= element.Value << bitShiftCursor;
        }
        return pipelineHash;
    }

    bool VkRenderer::CreateBuffer(uint32_t requestSizeInBytes, uint32_t usage, uint32_t flag, uint32_t queueFamilyIndex, bool bUseStaging, BufferInfo& outBufferInfo)
    {
        VkBufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = requestSizeInBytes;
        createInfo.pQueueFamilyIndices = &queueFamilyIndex;
        createInfo.queueFamilyIndexCount = 1;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.usage = usage;

        vkCreateBuffer(mDevice, &createInfo, nullptr, &outBufferInfo.Buffer);

        VkBufferMemoryRequirementsInfo2 bufferMemoryRequirements{};
        bufferMemoryRequirements.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
        bufferMemoryRequirements.buffer = outBufferInfo.Buffer;

        VkMemoryRequirements2 memoryRequirements2{};
        memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        vkGetBufferMemoryRequirements2(mDevice, &bufferMemoryRequirements, &memoryRequirements2);

        uint32_t memoryType = 0;
        uint32_t propertiesFlags = flag;
        if(bUseStaging)
        {
            propertiesFlags = (flag & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? (flag ^ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) : flag;
            propertiesFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            if (mbStagingEnabled == false && (flag & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
            {
                ASSERT(false, "staging 이 필요하지 않은 GPU일 수 있습니다.");
            }
        }

        getMemoryType(memoryRequirements2.memoryRequirements, propertiesFlags, memoryType);

        outBufferInfo.Alignment = memoryRequirements2.memoryRequirements.alignment;
        outBufferInfo.AllocationSize = memoryRequirements2.memoryRequirements.size;

        allocateMemory(outBufferInfo.AllocationSize, memoryType, outBufferInfo.Memory);

        VkBindBufferMemoryInfo bindBufferMemoryInfo{};
        bindBufferMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
        bindBufferMemoryInfo.buffer = outBufferInfo.Buffer;
        bindBufferMemoryInfo.memory = outBufferInfo.Memory;
        bindBufferMemoryInfo.memoryOffset = 0;
        vkBindBufferMemory2(mDevice, 1, &bindBufferMemoryInfo);

        return true;
    }


    void VkRenderer::AddVertexData(const void* const pData, uint32_t attribSize, uint32_t count, HashID hash, BufferRange& outRange)
    {
        if (mVertexBuffer.SubResources.find(hash) == mVertexBuffer.SubResources.end())
        {
            const uint32_t vertexSizeInBytes = count * attribSize;
            mVertexBuffer.Cursor = ALIGNED_SIZE(mVertexBuffer.Cursor, attribSize);
            const SubDataRange range{ mVertexBuffer.Cursor, vertexSizeInBytes };

            uploadBuffer(pData, vertexSizeInBytes, mVertexBuffer.Cursor, mVertexBuffer.Memory);

            outRange = BufferRange{ static_cast<int32_t>(mVertexBuffer.Cursor / attribSize) , static_cast<int32_t>(count) };
            mVertexBuffer.Cursor += vertexSizeInBytes;
            mVertexBuffer.SubResources.insert({ hash, std::move(range) });
        }
    }

    void VkRenderer::AddIndexData(void* pData, uint32_t attribSize, uint32_t count, HashID hash, BufferRange& outRange)
    {
        if (mIndexBuffer.SubResources.find(hash) == mIndexBuffer.SubResources.end())
        {
            const uint32_t indexSizeInBytes = count * attribSize;
            mIndexBuffer.Cursor = ALIGNED_SIZE(mIndexBuffer.Cursor, attribSize);
            const SubDataRange range{ mIndexBuffer.Cursor, indexSizeInBytes };

            uploadBuffer(pData, indexSizeInBytes, mIndexBuffer.Cursor, mIndexBuffer.Memory);
            outRange = BufferRange{ static_cast<int32_t>(mIndexBuffer.Cursor / attribSize) , static_cast<int32_t>(count) };

            mIndexBuffer.Cursor += indexSizeInBytes;
            mIndexBuffer.SubResources.insert({ hash, std::move(range) });
        }
    }

    void VkRenderer::copyBufferStaging(VkBuffer srcBuf, VkBuffer dstBuf, uint32_t copySize)
    {
        VkCommandPool commandPool;

        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.queueFamilyIndex = mGraphicsQueueIndex;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        vkCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &commandPool);

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.commandBufferCount = 1;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(mDevice, &commandBufferAllocateInfo, &commandBuffer);

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

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

        vkCmdCopyBuffer2(commandBuffer, &copyBufferInfo2);

        vkEndCommandBuffer(commandBuffer);

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
        commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferSubmitInfo.commandBuffer = commandBuffer;
        commandBufferSubmitInfo.deviceMask = 1;

        VkSubmitInfo2 submitInfo2{};
        submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo2.commandBufferInfoCount = 1;
        submitInfo2.pCommandBufferInfos = &commandBufferSubmitInfo;
        vkQueueSubmit2(mQueue, 1, &submitInfo2, nullptr);

        vkQueueWaitIdle(mQueue);

        vkDestroyCommandPool(mDevice, commandPool, nullptr);
    }

    void VkRenderer::copyBufferToImageStaging(VkBuffer srcBuf, VkImage dstImage, VkBufferImageCopy2& region)
    {
        VkCommandPool commandPool;
        createCommandPool(mGraphicsQueueIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, commandPool);

        VkCommandBuffer commandBuffer;
        createCommandBuffers(commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandBuffer);

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

        transitionImageLayoutNew(commandBuffer, dstImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {}, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
        VkCopyBufferToImageInfo2 copyBufferToImageInfo2{};
        copyBufferToImageInfo2.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
        copyBufferToImageInfo2.srcBuffer = srcBuf;
        copyBufferToImageInfo2.dstImage = dstImage;
        copyBufferToImageInfo2.pRegions = &region;
        copyBufferToImageInfo2.regionCount = 1;

        vkCmdCopyBufferToImage2(commandBuffer, &copyBufferToImageInfo2);

        transitionImageLayoutNew(commandBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

        vkEndCommandBuffer(commandBuffer);

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
        commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferSubmitInfo.commandBuffer = commandBuffer;
        commandBufferSubmitInfo.deviceMask = 1;

        VkSubmitInfo2 submitInfo2{};
        submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo2.commandBufferInfoCount = 1;
        submitInfo2.pCommandBufferInfos = &commandBufferSubmitInfo;
        vkQueueSubmit2(mQueue, 1, &submitInfo2, nullptr);

        vkQueueWaitIdle(mQueue);

        vkDestroyCommandPool(mDevice, commandPool, nullptr);
    }

    void VkRenderer::uploadBuffer(const void* const pData, uint32_t sizeInBytes, uint32_t offset, VkDeviceMemory memory)
    {
        VkMemoryMapInfo memoryMapInfo{};
        memoryMapInfo.sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO;
        memoryMapInfo.memory = memory;
        memoryMapInfo.size = sizeInBytes;
        memoryMapInfo.offset = offset;
        memoryMapInfo.flags = 0;

        void* pBufferCursor = nullptr;
        vkMapMemory2(mDevice, &memoryMapInfo, &pBufferCursor);

        (void)memcpy(pBufferCursor, pData, sizeInBytes);

        VkMemoryUnmapInfo memoryUnmapInfo{};
        memoryUnmapInfo.sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO;
        memoryUnmapInfo.memory = memory;
        vkUnmapMemory2(mDevice, &memoryUnmapInfo);
    }

    void VkRenderer::uploadBufferStaging(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t sizeInBytes, uint32_t srcOffset, uint32_t dstOffset)
    {
        VkCommandPool commandPool;
        createCommandPool(mGraphicsQueueIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, commandPool);
        VkCommandBuffer commandBuffer;
        createCommandBuffers(commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandBuffer);

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

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
        vkCmdCopyBuffer2(commandBuffer, &copyBufferInfo2);

        vkEndCommandBuffer(commandBuffer);

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
        commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferSubmitInfo.commandBuffer = commandBuffer;
        commandBufferSubmitInfo.deviceMask = 1;

        VkSubmitInfo2 submitInfo2{};
        submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo2.commandBufferInfoCount = 1;
        submitInfo2.pCommandBufferInfos = &commandBufferSubmitInfo;
        vkQueueSubmit2(mQueue, 1, &submitInfo2, nullptr);

        vkQueueWaitIdle(mQueue);

        vkDestroyCommandPool(mDevice, commandPool, nullptr);
    }

    bool VkRenderer::allocateMemory(uint32_t allocSize, uint32_t memoryType, VkDeviceMemory& outMemory)
    {
        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = allocSize;
        memoryAllocateInfo.memoryTypeIndex = memoryType;

        const VkResult result = vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &outMemory);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "AllocateMemory 실패 resultCode(%d)", result);
            return false;
        }
        return true;
    }

    bool VkRenderer::CreateTexture(const int8_t* const filePath, HashID& outHash, bool bIsDDS)
    {
        ASSERT(filePath != nullptr, "invalid path. (filePath is nullptr) ");
        ASSERT(*filePath != ' ', "invalid path. (filePath may empty string) ");
        ASSERT(*filePath != '\0', "invalid path. (filePath may empty(null) string) ");

        const HashID texHash = util::GetDjb2Hash(filePath);
        const auto& texIt = mTextures.find(texHash);
        if (texIt == mTextures.end())
        {
            wchar_t texFilePath[util::MAX_PATH_LENGTH];
            const uint32_t filePathLength = strlen(reinterpret_cast<char const*>(filePath));
            size_t  numConverted = 0;
            (void)mbstowcs_s(&numConverted, texFilePath, util::MAX_PATH_LENGTH, reinterpret_cast<char const*>(filePath), filePathLength);
            ASSERT(numConverted == (filePathLength + 1), "conversion result is incorrect. converted(%u) pathLength(%u)", numConverted, filePathLength);

            ScratchImage rawImage;
            if (FAILED(LoadFromWICFile(texFilePath, WIC_FLAGS_NONE, nullptr, rawImage)))
            {
                return false;
            }

            BufferInfo bufferInfo{};
            if(CreateBuffer(rawImage.GetImageCount(), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,mGraphicsQueueIndex, false, bufferInfo) == false)
            {
                return false;
            }

            TextureInfo newTexture{};
            // MEMO: staging buffer에 이미지 데이터 복사
            VkMemoryMapInfo memoryMapInfo{};
            memoryMapInfo.sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO;
            memoryMapInfo.memory = bufferInfo.Memory;
            memoryMapInfo.size = bufferInfo.AllocationSize;
            memoryMapInfo.offset = 0;

            void* pCursor = nullptr;
            vkMapMemory2(mDevice, &memoryMapInfo, &pCursor);

            (void)memcpy(pCursor, static_cast<void*>(rawImage.GetPixels()), rawImage.GetPixelsSize());

            VkMemoryUnmapInfo memoryUnmapInfo{};
            memoryUnmapInfo.sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO;
            memoryUnmapInfo.memory = bufferInfo.Memory;

            vkUnmapMemory2(mDevice, &memoryUnmapInfo);
            //
            // MEMO: 이미지 및 메모리 생성
            createImage(VK_IMAGE_LAYOUT_UNDEFINED, mGraphicsQueueIndex, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, { static_cast<uint32_t>(rawImage.GetMetadata().width), static_cast<uint32_t>(rawImage.GetMetadata().height), 1 }, bIsDDS ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, true, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newTexture.ImageMemory, newTexture.Image);

            // MEMO: staging buffer에서 이미지 메모리로 복사
            VkBufferImageCopy2 copyImageInfo2{};
            copyImageInfo2.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
            copyImageInfo2.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copyImageInfo2.imageOffset = {0, 0, 0};
            copyImageInfo2.imageExtent = { static_cast<uint32_t>(rawImage.GetMetadata().width), static_cast<uint32_t>(rawImage.GetMetadata().height) };
            copyImageInfo2.bufferImageHeight = 0;
            copyImageInfo2.bufferRowLength = 0;
            copyBufferToImageStaging(bufferInfo.Buffer, newTexture.Image, copyImageInfo2);


            VkImageViewCreateInfo imageViewCreateInfo{};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageViewCreateInfo.image = newTexture.Image;
            imageViewCreateInfo.viewType = {};
            vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &newTexture.ImageView);

            outHash = texHash;
        }
        else
        {
            outHash = texIt->first;
        }
        return true;
    }

    bool VkRenderer::createSamplers()
    {
        for(uint32_t samplerType = 0; samplerType < static_cast<uint32_t>(eSamplerType::SamplerCount); ++samplerType)
        {
            VkSamplerCreateInfo samplerCreateInfo{};
            samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerCreateInfo.mipLodBias= 0;
            samplerCreateInfo.anisotropyEnable = VK_FALSE;
            samplerCreateInfo.maxAnisotropy = 0.0f;
            samplerCreateInfo.compareEnable = VK_FALSE;
            samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

            VkSampler sampler;
            const VkResult result = vkCreateSampler(mDevice, &samplerCreateInfo, nullptr, &sampler);
            if(result != VK_SUCCESS)
            {
                ASSERT(false, "Sampler 생성 실패 sampler(%d), resultCode(%d)", sampler, result);
                return false;
            }

            mSamplers.insert({ static_cast<eSamplerType>(samplerType), sampler});
        }
        return true;
    }

    void VkRenderer::createImage(VkImageLayout layout, uint32_t queueFamilyIndex, VkImageUsageFlags usage, VkExtent3D imageExtent, VkImageType type, VkFormat format, bool bUseStaging, VkMemoryPropertyFlagBits memoryProperty, VkDeviceMemory& outImageMemory, VkImage& outImage)
    {
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.extent = imageExtent;
        imageCreateInfo.format = format;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.imageType = type;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = usage;
        imageCreateInfo.queueFamilyIndexCount = 1;
        imageCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = layout;
        vkCreateImage(mDevice, &imageCreateInfo, nullptr, &outImage);

        VkImageMemoryRequirementsInfo2 imageMemoryRequirements{};
        imageMemoryRequirements.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
        imageMemoryRequirements.image = outImage;

        VkMemoryRequirements2 memoryRequirements2{};
        vkGetImageMemoryRequirements2(mDevice, &imageMemoryRequirements, &memoryRequirements2);

        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
        getMemoryType(memoryRequirements2.memoryRequirements, memoryProperty, memoryAllocateInfo.memoryTypeIndex);
        vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &outImageMemory);

        VkBindImageMemoryInfo bindImageMemoryInfo{};
        bindImageMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
        bindImageMemoryInfo.memory = outImageMemory;
        bindImageMemoryInfo.memoryOffset = 0;
        bindImageMemoryInfo.image = outImage;
        vkBindImageMemory2(mDevice, 1, &bindImageMemoryInfo);
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

        // MEMO: 일단은 간단하게 처리
        mbStagingEnabled = mPhysicalDeviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
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

    bool VkRenderer::createImageViews()
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

    bool VkRenderer::createFrame()
    {
        ASSERT(mGraphicsQueueIndex != UINT_MAX, "mGraphicsQueueIndex가 유효하지 않습니다.");

        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (auto& frame : mFrames)
        {
            if (createCommandPool(mGraphicsQueueIndex, VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, frame.Pool) == false)
            {
                ASSERT(false, "CommandPool 생성 실패");
                return false;
            }

            if (createCommandBuffers(frame.Pool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, frame.Buffer) == false)
            {
                ASSERT(false, "CommandBuffer 생성 실패");
                return false;
            }

            VkResult result = vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &frame.ImageReadySemaphore);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "Semaphore(ImageReady) 생성 실패 resultCode(%d)", result);
                return false;
            }

            result = vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &frame.DrawFinishedFence);
            if (result != VK_SUCCESS)
            {
                ASSERT(false, "Fence 생성 실패 resultCode(%d)", result);
                return false;
            }
        }

        return true;
    }

    bool VkRenderer::createSyncObjects()
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

    bool VkRenderer::createGraphicsPipeline(eShader shader, HashID vsStageHash, HashID psStageHash, eVertexFormat vertexFormat, ePrimitiveTopology topology, eRasterType rasterType, eBlendState blendState, eDepthStencilState depthStencilState, eRenderPass renderPass, VkPipeline& outPipeline)
    {
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        VkDynamicState dynamicStates[] = { VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT, VkDynamicState::VK_DYNAMIC_STATE_SCISSOR };
        dynamicStateCreateInfo.dynamicStateCount = std::size(dynamicStates);
        dynamicStateCreateInfo.pDynamicStates = dynamicStates;


        VkVertexInputBindingDescription inputBindingDescription{};
        inputBindingDescription.stride = GetVertexStrideSize(vertexFormat);
        inputBindingDescription.binding = 0;
        inputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        constexpr VkVertexInputAttributeDescription VertexInputAttribTable[] =
        {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},   // P
            {1, 0, VK_FORMAT_R32G32_SFLOAT, 12},        // T
            {2, 0, VK_FORMAT_R32G32B32_SFLOAT, 20}   // N
        };
        constexpr uint8_t VertexInputAttribCountTable[] =
        {
            3,
            2,
            1
        };

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexInputAttribTable;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = VertexInputAttribCountTable[static_cast<uint8_t>(vertexFormat)];
        vertexInputStateCreateInfo.pVertexBindingDescriptions = &inputBindingDescription;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;

        const auto& vsStageInfoIt = mShaderInfos.find(vsStageHash);
        const auto& psStageInfoIt = mShaderInfos.find(psStageHash);
        const VkPipelineShaderStageCreateInfo ShaderStageInfos[] =
        {
            vsStageInfoIt->second.PipelineShaderStage,
            psStageInfoIt->second.PipelineShaderStage
        };

        constexpr VkPrimitiveTopology TopologyMap[] =
        {
            VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST
        };
        static_assert(std::size(TopologyMap) == static_cast<uint8_t>(ePrimitiveTopology::TopologyCount), "TopologyMap와 ePrimitiveTopology의 요소 수가 일치해야 합니다.");
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.topology = TopologyMap[static_cast<uint8_t>(topology)];

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        const VkViewport viewport = VkViewport{ 0.0f, 0.0f, static_cast<float>(mSwapChainExtent.width), static_cast<float>(mSwapChainExtent.height), 0.0f, 1.0f };
        viewportStateCreateInfo.pViewports = &viewport;
        const VkRect2D scissorRange{ {0,0}, mSwapChainExtent };
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissorRange;

        constexpr VkCullModeFlagBits RasterTypeTable[] =
        {
             VkCullModeFlagBits::VK_CULL_MODE_NONE,
             VkCullModeFlagBits::VK_CULL_MODE_FRONT_BIT,
             VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT,
             VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT,
        };
        static_assert(std::size(RasterTypeTable) == static_cast<uint8_t>(eRasterType::RasterCount), "RasterTypeTable와 eRasterType의 요소 수가 일치해야 합니다.");
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
        rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.cullMode = RasterTypeTable[static_cast<uint8_t>(rasterType)];
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationStateCreateInfo.lineWidth = 1.0f;
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;

        constexpr VkColorComponentFlags COLOR_COMPONENT_ALL = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        constexpr VkPipelineColorBlendAttachmentState BlendStateTable[] =
        {
            {
	            VK_FALSE,
            	VK_BLEND_FACTOR_ONE,VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
            	VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
            	COLOR_COMPONENT_ALL
            },
        {
        	VK_TRUE,
        	VK_BLEND_FACTOR_SRC_ALPHA,VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        	VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE,
        	VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
        	COLOR_COMPONENT_ALL
        },
        };
        static_assert(std::size(BlendStateTable) == static_cast<uint8_t>(eBlendState::StateCount), "BlendStateTable와 eBlendState의 요소 수가 일치해야 합니다.");

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
        colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.pAttachments = &BlendStateTable[static_cast<uint8_t>(blendState)];
        colorBlendStateCreateInfo.attachmentCount = 1;


        constexpr VkPipelineDepthStencilStateCreateInfo DepthStencilStateTable[] =
        {
            {{}, nullptr,{}, VK_FALSE, VK_FALSE, {}, VK_FALSE, VK_FALSE, {}, {}, {}, {}},
            {{}, nullptr,{}, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE, VK_FALSE, {}, {}, {}, {}},
        };
        static_assert(std::size(DepthStencilStateTable) == static_cast<uint8_t>(eDepthStencilState::StateCount), "DepthStencilStateTable와 eDepthStencilState의 요소 수가 일치해야 합니다.");

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.stencilTestEnable = DepthStencilStateTable[static_cast<uint8_t>(depthStencilState)].stencilTestEnable;
        depthStencilStateCreateInfo.depthTestEnable = DepthStencilStateTable[static_cast<uint8_t>(depthStencilState)].depthTestEnable;
        depthStencilStateCreateInfo.depthWriteEnable = DepthStencilStateTable[static_cast<uint8_t>(depthStencilState)].depthWriteEnable;
        depthStencilStateCreateInfo.depthCompareOp = DepthStencilStateTable[static_cast<uint8_t>(depthStencilState)].depthCompareOp;
        depthStencilStateCreateInfo.depthBoundsTestEnable = DepthStencilStateTable[static_cast<uint8_t>(depthStencilState)].depthBoundsTestEnable;


        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;

        const auto& pipelineLayoutIt = mShaderLayouts.find(shader);

        VkPipelineRenderingCreateInfo renderingCreateInfo{};
        renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingCreateInfo.colorAttachmentCount = 1;
        renderingCreateInfo.pColorAttachmentFormats = &mSurfaceFormat.format;

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pNext = &renderingCreateInfo;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pDynamicState= &dynamicStateCreateInfo;
        pipelineCreateInfo.pStages = ShaderStageInfos;
        pipelineCreateInfo.stageCount = std::size(ShaderStageInfos);
        pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        pipelineCreateInfo.layout = pipelineLayoutIt->second.PipelineLayout;

        const VkResult result = vkCreateGraphicsPipelines(mDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &outPipeline);
        if(result != VK_SUCCESS)
        {
            ASSERT(false, "pipeline 생성 실패 resultCode(%d)", result);
            return false;
        }
	    return true;
    }

    bool VkRenderer::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flag, VkCommandPool& outCommandPool) const
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

    bool VkRenderer::createCommandBuffers(VkCommandPool commandPool, uint32_t bufferCount, VkCommandBufferLevel level, VkCommandBuffer& outBuffers) const
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

    uint32_t VkRenderer::acquireSwapChainImage(const PerFrame& frame)
    {
        uint32_t nextImageIndex = UINT32_MAX;
        VkAcquireNextImageInfoKHR acqNextImageInfo{};
        acqNextImageInfo.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
        acqNextImageInfo.semaphore = frame.ImageReadySemaphore;
        acqNextImageInfo.swapchain = mSwapChain;
        acqNextImageInfo.timeout = 1'000'000'000; // 1sec
        acqNextImageInfo.deviceMask = 1;

        const VkResult result = vkAcquireNextImage2KHR(mDevice, &acqNextImageInfo, &nextImageIndex);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "next image 획득 실패 resultCode(%d)", result);
        }
        ASSERT(nextImageIndex < mImages.size(), "유효하지 않은 image Index를 획득했거나, index 획득에 실패했습니다.  nextImageIndex(%u)", nextImageIndex);
        return nextImageIndex;
    }

    void VkRenderer::transitionImageLayout(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask)
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

    void VkRenderer::transitionImageLayoutNew(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask)
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

    VkRenderer::PerFrame VkRenderer::getCurrentFrameResource() const
    {
        const uint32_t curFrameIndex = mFrameCount % sOverlappedFrame;
        return mFrames[curFrameIndex];
    }

    uint32_t VkRenderer::getCurrentFrameIndex() const
    {
        return mFrameCount % sOverlappedFrame;
    }

    void VkRenderer::getMemoryType(VkMemoryRequirements memoryRequirements, uint32_t flag, uint32_t& outMemoryType)
    {
        VkPhysicalDeviceMemoryProperties2 physicalDeviceMemoryProperties2{};
        physicalDeviceMemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

        vkGetPhysicalDeviceMemoryProperties2(mPhysicalDevice, &physicalDeviceMemoryProperties2);

        for (uint32_t bit = 0; bit < physicalDeviceMemoryProperties2.memoryProperties.memoryTypeCount; ++bit)
        {
            if (memoryRequirements.memoryTypeBits & (1 << bit) &&
                physicalDeviceMemoryProperties2.memoryProperties.memoryTypes[bit].propertyFlags == flag)
            {
                outMemoryType = bit;
            }
        }
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
