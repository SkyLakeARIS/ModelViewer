#include "PipelineBuilder.h"
#include "../../Util/Macro.h"
#include "../Shader/Shader.h"
#include "../Shader/ShaderModule.h"

namespace renderer
{
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

    PipelineBuilder::PipelineBuilder(const VkDevice& device)
        : mDevice(device)
        , mViewport()
        , mScissorExtent()
        , mViewportState()
        , mDynamicState()
        , mShaderStageInfos{}
        , mInputAssemblyStateCreateInfo()
        , mVertexInputBindingDescription()
        , mVertexInputState()
        , mMultisampleStateCreateInfo()
        , mDepthStencilState()
        , mBlendAttachmentState()
        , mColorBlendState()
        , mRasterizationState()
        , mPipelineLayout(nullptr)
        , mRenderingCreateInfo()
        , mPipelineCreateInfo()
        , mValidation()
        , mHashComponents()
    {
    }

    PipelineBuilder::~PipelineBuilder()
    {
        mDynamicStates.clear();
        std::vector<VkDynamicState>().swap(mDynamicStates);
        mColorAttachmentFormats.clear();
        std::vector<VkFormat>().swap(mColorAttachmentFormats);
        mVertexInputAttributeDescriptions.clear();
        std::vector<VkVertexInputAttributeDescription>().swap(mVertexInputAttributeDescriptions);
    }

    void PipelineBuilder::SetViewportState(uint32_t viewportWidth, uint32_t viewportHeight)
    {
        mViewport = VkViewport{ 0.0f, 0.0f, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight), 0.0f, 1.0f };
        mScissorExtent = VkRect2D{ {0,0},{viewportWidth, viewportHeight} };

        mViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        mViewportState.viewportCount = 1;
        mViewportState.pViewports = &mViewport;
        mViewportState.scissorCount = 1;
        mViewportState.pScissors = &mScissorExtent;
        mValidation.bViewportStateValid = true;
    }

    void PipelineBuilder::SetDynamicState(bool bViewport, bool bScissor)
    {
        ASSERT(mDynamicStates.empty(), "SetDynamicStateState가 두 번 호출되었습니다. 기존 내용을 버립니다.");
        mDynamicStates.clear();

        if(bViewport)
        {
            mDynamicStates.push_back(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT);
        }

        if(bScissor)
        {
            mDynamicStates.push_back(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR);
        }
        mDynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        mDynamicState.dynamicStateCount = mDynamicStates.size();
        mDynamicState.pDynamicStates = mDynamicStates.data();
        mValidation.bDynamicStateValid = true;
    }

    void PipelineBuilder::SetShaderStage(const Shader& shader)
    {
        mShaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        mShaderStageInfos[0].stage = shader.GetVs()->GetShaderStageType() == eShaderStageType::Pixel ? VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT : VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
        mShaderStageInfos[0].module = shader.GetVs()->GetShaderModule();
        mShaderStageInfos[0].pName = ShaderModule::sEntryPoint;

        mShaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        mShaderStageInfos[1].stage = shader.GetPs()->GetShaderStageType() == eShaderStageType::Pixel ? VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT : VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
        mShaderStageInfos[1].module = shader.GetPs()->GetShaderModule();
        mShaderStageInfos[1].pName = ShaderModule::sEntryPoint;

        mHashComponents.Shader = shader.GetShaderType();
        mValidation.bShaderStageValid = true;
    }

    void PipelineBuilder::SetInputAssemblyState(ePrimitiveTopology topology)
    {
        constexpr VkPrimitiveTopology TopologyMap[] =
        {
            VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST
        };
        static_assert(std::size(TopologyMap) == static_cast<uint8_t>(ePrimitiveTopology::TopologyCount), "TopologyMap와 ePrimitiveTopology의 요소 수가 일치해야 합니다.");
        mInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        mInputAssemblyStateCreateInfo.topology = TopologyMap[static_cast<uint8_t>(topology)];

        mHashComponents.PrimitiveTopology = topology;
        mValidation.bInputAssemblyValid = true;
    }

    void PipelineBuilder::SetVertexInputState(eVertexFormat vertexFormat)
    {
        ASSERT(mVertexInputAttributeDescriptions.empty(), "SetVertexInputState가 두 번 호출되었습니다. 기존 내용을 버립니다.");
        mVertexInputAttributeDescriptions.clear();

        mVertexInputBindingDescription.stride = GetVertexStrideSize(vertexFormat);
        mVertexInputBindingDescription.binding = 0;
        mVertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

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

        mVertexInputAttributeDescriptions.resize(VertexInputAttribCountTable[static_cast<uint8_t>(vertexFormat)]);

        (void)memcpy(mVertexInputAttributeDescriptions.data(), VertexInputAttribTable, sizeof(VkVertexInputAttributeDescription) * VertexInputAttribCountTable[static_cast<uint8_t>(vertexFormat)]);

        mVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        mVertexInputState.pVertexAttributeDescriptions = mVertexInputAttributeDescriptions.data();
        mVertexInputState.vertexAttributeDescriptionCount = mVertexInputAttributeDescriptions.size();
        mVertexInputState.pVertexBindingDescriptions = &mVertexInputBindingDescription;
        mVertexInputState.vertexBindingDescriptionCount = 1;

        mHashComponents.VertexFormat = vertexFormat;
        mValidation.bVertexInputValid = true;
    }

    void PipelineBuilder::SetMultiSamplerState(bool bEnable)
    {
        if(bEnable)
        {
            mMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            mMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
            mMultisampleStateCreateInfo.sampleShadingEnable = VK_TRUE;
        }
        else
        {
            mMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            mMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
            mMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        }
        mValidation.bMultiSamplerValid = true;
    }

    void PipelineBuilder::SetDepthStencilState(eDepthStencilState depthStencilState)
    {
        constexpr VkPipelineDepthStencilStateCreateInfo DepthStencilStateTable[] =
        {
            {{}, nullptr,{}, VK_FALSE, VK_FALSE, {}, VK_FALSE, VK_FALSE, {}, {}, {}, {}},
            {{}, nullptr,{}, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE, VK_FALSE, {}, {}, {}, {}},
        };
        static_assert(std::size(DepthStencilStateTable) == static_cast<uint8_t>(eDepthStencilState::StateCount), "DepthStencilStateTable와 eDepthStencilState의 요소 수가 일치해야 합니다.");

        mDepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        mDepthStencilState.stencilTestEnable = DepthStencilStateTable[static_cast<uint8_t>(depthStencilState)].stencilTestEnable;
        mDepthStencilState.depthTestEnable = DepthStencilStateTable[static_cast<uint8_t>(depthStencilState)].depthTestEnable;
        mDepthStencilState.depthWriteEnable = DepthStencilStateTable[static_cast<uint8_t>(depthStencilState)].depthWriteEnable;
        mDepthStencilState.depthCompareOp = DepthStencilStateTable[static_cast<uint8_t>(depthStencilState)].depthCompareOp;
        mDepthStencilState.depthBoundsTestEnable = DepthStencilStateTable[static_cast<uint8_t>(depthStencilState)].depthBoundsTestEnable;

        mHashComponents.DepthStencilState = depthStencilState;
        mValidation.bDepthStencilValid = true;
    }

    void PipelineBuilder::SetColorBlendState(eBlendState blendState)
    {
        ASSERT(mValidation.bRenderPassValid, "SetRenderPass 함수에 의존성이 있으므로 먼저 호출되어 RenderPass 정보가 설정되어야 합니다.");
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

        (void)memcpy(&mBlendAttachmentState, &BlendStateTable[static_cast<uint8_t>(blendState)], sizeof(VkPipelineColorBlendAttachmentState));
        
        mColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        mColorBlendState.pAttachments = &mBlendAttachmentState;
        mColorBlendState.attachmentCount = mHashComponents.RenderPass == eRenderPass::Shadow ? 0 : 1;

        mHashComponents.BlendState = blendState;
        mValidation.bColorBlendValid = true;
    }

    void PipelineBuilder::SetRasterizationState(eRasterType rasterType)
    {
        constexpr VkCullModeFlagBits RasterTypeTable[] =
        {
             VkCullModeFlagBits::VK_CULL_MODE_NONE,
             VkCullModeFlagBits::VK_CULL_MODE_FRONT_BIT,
             VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT,
             VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT,
        };
        static_assert(std::size(RasterTypeTable) == static_cast<uint8_t>(eRasterType::RasterCount), "RasterTypeTable와 eRasterType의 요소 수가 일치해야 합니다.");

        mRasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        mRasterizationState.cullMode = RasterTypeTable[static_cast<uint8_t>(rasterType)];
        mRasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
        mRasterizationState.lineWidth = 1.0f;
        mRasterizationState.polygonMode = VK_POLYGON_MODE_FILL;

        mHashComponents.RasterType = rasterType;
        mValidation.bRasterizationValid = true;
    }

    void PipelineBuilder::SetPipelineLayout(const VkPipelineLayout& pipelineLayout)
    {
        mPipelineLayout = pipelineLayout;
        mValidation.bPipelineLayoutValid = true;
    }

    void PipelineBuilder::SetColorAttachInfo(const std::vector<VkFormat>& formats, VkFormat depthStencilFormat)
    {
        ASSERT(mColorAttachmentFormats.empty(), "SetColorAttachInfo가 두 번 호출되었습니다. 기존 내용을 버립니다.");
        mColorAttachmentFormats.clear();
        if (formats.size())
        {
            mColorAttachmentFormats.resize(formats.size());
            (void)memcpy(mColorAttachmentFormats.data(), formats.data(), sizeof(VkFormat) * formats.size());
        }

        mRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        mRenderingCreateInfo.colorAttachmentCount = mColorAttachmentFormats.size();
        mRenderingCreateInfo.pColorAttachmentFormats = mColorAttachmentFormats.data();
        mRenderingCreateInfo.depthAttachmentFormat = depthStencilFormat;
        mValidation.bColorAttachValid = true;
    }

    void PipelineBuilder::SetRenderPass(eRenderPass renderPass)
    {
        mHashComponents.RenderPass = renderPass;
        mValidation.bRenderPassValid = true;
    }

    bool PipelineBuilder::Create(VkPipeline& outPipeline, HashID& outPipelineHash)
    {
        ASSERT(mValidation.bViewportStateValid, "ViewportState 정보가 세팅되지 않았습니다.");
        ASSERT(mValidation.bDynamicStateValid, "DynamicState 정보가 세팅되지 않았습니다.");
        ASSERT(mValidation.bShaderStageValid, "ShaderStage 정보가 세팅되지 않았습니다.");
        ASSERT(mValidation.bInputAssemblyValid, "InputAssemblyState 정보가 세팅되지 않았습니다.");
        ASSERT(mValidation.bVertexInputValid, "VertexInputState 정보가 세팅되지 않았습니다.");
        ASSERT(mValidation.bMultiSamplerValid, "MultiSamplerState 정보가 세팅되지 않았습니다.");
        ASSERT(mValidation.bDepthStencilValid, "DepthStencilState 정보가 세팅되지 않았습니다.");
        ASSERT(mValidation.bColorBlendValid, "ColorBlendState 정보가 세팅되지 않았습니다.");
        ASSERT(mValidation.bRasterizationValid, "RasterizationState 정보가 세팅되지 않았습니다.");
        ASSERT(mValidation.bPipelineLayoutValid, "PipelineLayout 정보가 세팅되지 않았습니다.");
        ASSERT(mValidation.bColorAttachValid, "ColorAttachInfo 정보가 세팅되지 않았습니다.");
        ASSERT(mValidation.bRenderPassValid, "RenderPass 정보가 세팅되지 않았습니다. PipelineHash가 올바르게 계산되지 않을 수 있습니다.");

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pNext = &mRenderingCreateInfo;
        pipelineCreateInfo.pViewportState = &mViewportState;
        pipelineCreateInfo.pDynamicState = &mDynamicState;
        pipelineCreateInfo.pStages = mShaderStageInfos;
        pipelineCreateInfo.stageCount = std::size(mShaderStageInfos);
        pipelineCreateInfo.pVertexInputState = &mVertexInputState;
        pipelineCreateInfo.pInputAssemblyState = &mInputAssemblyStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &mRasterizationState;
        pipelineCreateInfo.pColorBlendState = &mColorBlendState;
        pipelineCreateInfo.pDepthStencilState = &mDepthStencilState;
        pipelineCreateInfo.pMultisampleState = &mMultisampleStateCreateInfo;
        pipelineCreateInfo.layout = mPipelineLayout;


        const VkResult result = vkCreateGraphicsPipelines(mDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &outPipeline);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "pipeline 생성 실패 resultCode(%d)", result);
            return false;
        }

        outPipelineHash = makePipelineHash(mHashComponents);

        return true;
    }

    void PipelineBuilder::Release()
    {
        // MEMO: Pooling 시에 활용할 리셋 함수
    }

    HashID PipelineBuilder::PeekPipelineHash(eShader shader, eVertexFormat vertexFormat, ePrimitiveTopology topology, eRasterType rasterType, eBlendState blendState, eDepthStencilState depthStencilState, eRenderPass renderPass)
    {
        PipelineHashComponents hashComponents;
        hashComponents.RenderPass = renderPass;
        hashComponents.RasterType = rasterType;
        hashComponents.BlendState = blendState;
        hashComponents.DepthStencilState = depthStencilState;
        hashComponents.VertexFormat = vertexFormat;
        hashComponents.Shader = shader;
        hashComponents.PrimitiveTopology = topology;
        return makePipelineHash(hashComponents);
    }

    HashID PipelineBuilder::makePipelineHash(const PipelineHashComponents& hashComponents)
    {
        struct PipelineStateBitField
        {
            HashID Value;
            HashID BitWidth;
        };

        const PipelineStateBitField PipelineBitLayout[] =
        {
            {static_cast<HashID>(hashComponents.Shader), GetBitCount(eShader::ShaderCount)},
            {static_cast<HashID>(hashComponents.VertexFormat), GetBitCount(eVertexFormat::FormatCount)},
            {static_cast<HashID>(hashComponents.PrimitiveTopology), GetBitCount(ePrimitiveTopology::TopologyCount)},
            {static_cast<HashID>(hashComponents.RasterType), GetBitCount(eRasterType::RasterCount)},
            {static_cast<HashID>(hashComponents.BlendState), GetBitCount(eBlendState::StateCount)},
            {static_cast<HashID>(hashComponents.DepthStencilState), GetBitCount(eDepthStencilState::StateCount)},
            {static_cast<HashID>(hashComponents.RenderPass), GetBitCount(eRenderPass::PassCount)},
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
}
