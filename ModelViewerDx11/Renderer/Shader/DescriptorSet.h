#pragma once
#include "../GfxPrerequisites.h"
#include "ShaderType.h"
#include "../../Util/Type.h"

namespace renderer
{
    enum class eTextureType : uint8_t;
    class Sampler;
}

namespace renderer
{
    class Texture;
    class Shader;
    enum class eShader : uint8_t;
    class Buffer;

    struct PoolInfo
    {
        uint16_t PoolIndex;
        HashID LayoutHash; // MEMO: (아이디어 메모)일단 간단하게 하고, 나중에 Pool에서 관리하고 있는 어떤 key값으로 반환. key는 어떤 layout인지 + 어떤 PoolIndex인지 정보 조합
    };

    struct TextureWriteInfo
    {
        eTextureType TextureType;
        Texture* Texture;
        Sampler* Sampler;
    };
    // MEMO: 현재 문제점: DescriptorSetLayout 정비가 안되어 있어서 중복된 Descriptor들을 업로드/Pool사이즈 계산에 비효율적인 부분이 있음.
    // ShaderSources/Shader/DescriptorSet 부분은 작업 대상
    // MEMO: 현재 문제점: SetLayout 구분 필요 (프레임별, 물체별)
    class DescriptorSet
    {
    public:
        DescriptorSet(const VkDevice& device, const std::vector<VkDescriptorSet>& descriptorSets, const PoolInfo& ownerInfo);
        ~DescriptorSet();

        bool Create();
        void UpdateDescriptorSet(eShader shader);
        void UpdateUniformData(const void* const pData, uint32_t count, uint32_t elementSize, uint32_t inFlightIndex, bool bPerFrame);
        VkDescriptorSet GetDescriptorSet(uint32_t inFlightIndex) const;
        void GetLayoutWriteInfos(eShader shaderType, std::vector<TextureWriteInfo>& outTextureWriteInfos);
    private:
        VkDevice mDevice;
        std::vector<Buffer*> mBuffers;
        std::vector<VkDescriptorSet> mDescriptorSets;
        PoolInfo mOwnerInfo;
    };
}

