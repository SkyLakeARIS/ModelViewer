#pragma once
#include "../GfxPrerequisites.h"

namespace renderer
{
    enum class eShader : uint8_t;

    enum class eLayoutType : uint8_t
    {
        Common,
        Material,
        Deferred,
        LayoutCount
    };

    struct DescriptorInfo
    {
        VkDescriptorType Type;
        uint32_t Count;
    };

    class DescriptorLayout
    {
    private:
        struct LayoutPair
        {
            VkDescriptorSetLayout SetLayout;
            VkPipelineLayout PipelineLayout;
        };
    public:
        static bool Initialize(VkDevice device);
        static void Release();

        static eLayoutType GetLayoutTypeByShaderType(eShader shader);
        static void GetLayoutWriteInfos();
        static const std::vector<DescriptorInfo>& GetDescriptorInfos(eLayoutType type);
        static VkDescriptorSetLayout GetDescriptorSetLayout(eLayoutType type);
        static VkPipelineLayout GetPipelineLayout(eLayoutType type);
    private:
        DescriptorLayout() = default;
        ~DescriptorLayout() = default;
    private:
        static VkDevice sDevice;
        static std::unordered_map<eLayoutType, LayoutPair> sLayoutPairs;
        static std::unordered_map<eLayoutType, std::vector<DescriptorInfo>> sDescriptorInfos;
    };
}
