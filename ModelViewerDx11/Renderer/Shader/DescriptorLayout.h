#pragma once
#include "../GfxPrerequisites.h"

namespace renderer
{
    enum class eShader : uint8_t;
    enum class eTextureType : uint8_t;
    class Texture;
    class Sampler;
    class Buffer;

    enum class eLayoutType : uint8_t
    {
        Common,
        Material,
        Deferred,
        LayoutCount
    };

    struct DescriptorInfo
    {
        uint32_t Binding;
        VkDescriptorType Type;
        uint32_t Count;
    };

    struct DescriptorWriteInfo
    {
        DescriptorInfo Descriptor;
        bool bTexture;
        eTextureType TextureType; // MEMO: bTexture일 때만 유효. 아니면 TextureTypeCount
        VkImageView ImageView;
        Sampler* Sampler;
        Buffer* Buffer;
        VkDeviceSize Offset;
        VkDeviceSize Range;
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
        static void GetLayoutWriteInfos(eLayoutType layoutType, std::vector<DescriptorWriteInfo>& outWriteInfos);
        static void GetLayoutWriteInfosByShader(eLayoutType layoutType, eShader shaderType, std::vector<DescriptorWriteInfo>& outWriteInfos);
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
