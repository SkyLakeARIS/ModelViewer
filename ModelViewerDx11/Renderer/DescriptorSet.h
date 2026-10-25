#pragma once
#include "GfxPrerequisites.h"
#include "Shader/ShaderType.h"

namespace renderer
{
    class Shader;
    enum class eShader : uint8_t;
    class Buffer;

    // MEMO: 현재 문제점: DescriptorSetLayout 정비가 안되어 있어서 중복된 Descriptor들을 업로드/Pool사이즈 계산에 비효율적인 부분이 있음.
    // ShaderSources/Shader/DescriptorSet 부분은 작업 대상
    // MEMO: 현재 문제점: SetLayout 구분 필요 (프레임별, 물체별)
    class DescriptorSet
    {
    public:
        DescriptorSet(const VkDevice& device);
        ~DescriptorSet();

        bool Create(uint32_t frameOverlap, const Shader* const shader);
        bool UpdateDescriptorSet(eShader shader, uint32_t inFlightIndex);
        void UpdateUniformData(const void* const pData, uint32_t count, uint32_t elementSize, uint32_t inFlightIndex, bool bPerFrame);
        VkDescriptorSet GetDescriptorSet(uint32_t inFlightIndex) const;
    private:
        VkDevice mDevice;
        std::vector<Buffer*> mBuffers;
        VkDescriptorPool mDescriptorPool;
        std::vector<VkDescriptorSet> mDescriptorSets;
    };
}

