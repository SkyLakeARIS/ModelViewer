#pragma once
#include "../GfxPrerequisites.h"
#include "ShaderType.h"
#include "../../Util/Type.h"

namespace renderer
{
    struct DescriptorWriteInfo;
}

namespace renderer
{
    enum class eTextureType : uint8_t;
    enum class eLayoutType : uint8_t;
    class Sampler;
    class Texture;
    class Shader;
    class Buffer;

    struct PoolInfo
    {
        uint16_t PoolIndex;
        eLayoutType LayoutType;
    };

    struct TextureWriteInfo
    {
        eTextureType TextureType;
        Texture* Texture;
        Sampler* Sampler;
    };

    class DescriptorSet
    {
    public:
        DescriptorSet(const VkDevice& device, const std::vector<VkDescriptorSet>& descriptorSets, const PoolInfo& ownerInfo);
        ~DescriptorSet();

        bool Create();
        void UpdateDescriptorSet(const std::vector<DescriptorWriteInfo>& writeInfos);
        void UpdateUniformData(const void* const pData, uint32_t count, uint32_t elementSize, uint32_t inFlightIndex);
        VkDescriptorSet GetDescriptorSet(uint32_t inFlightIndex) const;
        void GetLayoutWriteInfos(eShader shaderType, std::vector<TextureWriteInfo>& outTextureWriteInfos);
        eLayoutType GetLayoutType() const;
    private:
        VkDevice mDevice;
        std::vector<Buffer*> mBuffers;
        std::vector<VkDescriptorSet> mDescriptorSets;
        PoolInfo mOwnerInfo;
    };
}

