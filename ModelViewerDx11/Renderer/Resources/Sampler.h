#pragma once
#include "RenderTypes.h"
#include "../GfxPrerequisites.h"

namespace renderer
{
    class Sampler
    {
    public:
        Sampler(const VkDevice& device);
        ~Sampler();

        bool Create(eSamplerType type);
        void Release();

        VkSampler GetSampler() const;
    private:
        VkDevice mDevice;
        VkSampler mSampler;
        eSamplerType mType;
    };
}
