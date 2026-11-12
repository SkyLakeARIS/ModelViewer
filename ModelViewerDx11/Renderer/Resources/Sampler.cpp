#include "Sampler.h"
#include "../../Util/Macro.h"
#include "../../Util/Type.h"

namespace renderer
{
    Sampler::Sampler(const VkDevice& device)
        : mDevice(device)
        , mSampler(nullptr)
        , mType()
    {
    }

    Sampler::~Sampler()
    {
        vkDestroySampler(mDevice, mSampler, nullptr);
    }

    bool Sampler::Create(eSamplerType type)
    {
        VkSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.mipLodBias = 0;
        samplerCreateInfo.anisotropyEnable = VK_FALSE;
        samplerCreateInfo.maxAnisotropy = 0.0f;
        samplerCreateInfo.compareEnable = VK_FALSE;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        const VkResult result = vkCreateSampler(mDevice, &samplerCreateInfo, nullptr, &mSampler);
        if (result != VK_SUCCESS)
        {
            ASSERT(false, "Sampler 생성 실패 type(%d), resultCode(%d)", static_cast<EnumInt>(type), result);
            return false;
        }

        return true;
    }

    void Sampler::Release()
    {
        // MEMO: Pooling 시에 활용할 리셋 함수
    }

    VkSampler Sampler::GetSampler() const
    {
        return mSampler;
    }
}
