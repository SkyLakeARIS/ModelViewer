#pragma once
#include "RenderTypes.h"
#include "VertexType.h"
#include "../GfxPrerequisites.h"
#include "../../Util/Type.h"

namespace renderer
{
    class Shader;
    enum class eShader : uint8_t;

    class PipelineBuilder
    {
    private:
        struct BuilderValidation
        {
            bool bViewportStateValid;
            bool bDynamicStateValid;
            bool bShaderStageValid;
            bool bInputAssemblyValid;
            bool bVertexInputValid;
            bool bMultiSamplerValid;
            bool bDepthStencilValid;
            bool bColorBlendValid;
            bool bRasterizationValid;
            bool bPipelineLayoutValid;
            bool bColorAttachValid;
            bool bRenderPassValid;
        };

        struct PipelineHashComponents
        {
            eShader Shader;
            eVertexFormat VertexFormat;
            ePrimitiveTopology PrimitiveTopology;
            eRasterType RasterType;
            eBlendState BlendState;
            eDepthStencilState DepthStencilState;
            eRenderPass RenderPass;
        };
    public:
        PipelineBuilder(const VkDevice& device);
        ~PipelineBuilder();

        void SetViewportState(uint32_t viewportWidth, uint32_t viewportHeight);
        void SetDynamicState(bool bViewport, bool bScissor);
        void SetShaderStage(const Shader& shader);
        void SetInputAssemblyState(ePrimitiveTopology topology);
        void SetVertexInputState(eVertexFormat vertexFormat);
        void SetMultiSamplerState(bool bEnable);
        void SetDepthStencilState(eDepthStencilState depthStencilState);
        void SetColorBlendState(eBlendState blendState);
        void SetRasterizationState(eRasterType rasterType);
        void SetPipelineLayout(const VkPipelineLayout& pipelineLayout);
        void SetColorAttachInfo(const std::vector<VkFormat>& formats, VkFormat depthStencilFormat);

        void SetRenderPass(eRenderPass renderPass);

        bool Create(VkPipeline& outPipeline, HashID& outPipelineHash);
        void Release();

        HashID PeekPipelineHash(eShader shader, eVertexFormat vertexFormat, ePrimitiveTopology topology, eRasterType rasterType, eBlendState blendState, eDepthStencilState depthStencilState, eRenderPass renderPass);
    private:

        HashID makePipelineHash(const PipelineHashComponents& hashComponents);
    private:
        VkDevice mDevice;

        VkViewport mViewport;
        VkRect2D mScissorExtent;
        VkPipelineViewportStateCreateInfo mViewportState;

        std::vector<VkDynamicState> mDynamicStates;
        VkPipelineDynamicStateCreateInfo mDynamicState;

        VkPipelineShaderStageCreateInfo mShaderStageInfos[2];

        VkPipelineInputAssemblyStateCreateInfo mInputAssemblyStateCreateInfo;

        VkVertexInputBindingDescription mVertexInputBindingDescription;
        std::vector<VkVertexInputAttributeDescription> mVertexInputAttributeDescriptions;
        VkPipelineVertexInputStateCreateInfo mVertexInputState;

        VkPipelineMultisampleStateCreateInfo mMultisampleStateCreateInfo;

        VkPipelineDepthStencilStateCreateInfo mDepthStencilState;

        VkPipelineColorBlendAttachmentState mBlendAttachmentState;
        VkPipelineColorBlendStateCreateInfo mColorBlendState;

        VkPipelineRasterizationStateCreateInfo mRasterizationState;

        VkPipelineLayout mPipelineLayout;

        std::vector<VkFormat> mColorAttachmentFormats;
        VkPipelineRenderingCreateInfo mRenderingCreateInfo;

        //VkPipelineTessellationStateCreateInfo mTessellationState; // MEMO: Reserved

        VkGraphicsPipelineCreateInfo mPipelineCreateInfo;
        BuilderValidation mValidation;
        PipelineHashComponents mHashComponents;
    };

}
