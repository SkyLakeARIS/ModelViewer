#pragma once
#include <atomic>
#include "../GfxPrerequisites.h"
#include "../../Util/Type.h"

namespace renderer
{
    class Image;

    class Texture
    {
    public:
        Texture(const VkDevice& device);
        ~Texture();

        bool Create(const int8_t* const filePath, bool bIsDDS);
        void Release();

        VkImageView GetImageView() const;
    private:
        bool loadImage(const int8_t* const filePath, DirectX::ScratchImage& outImage);
    private:
        VkDevice mDevice;
        Image* mImage;
        VkImageView mImageView;

        std::atomic_int32_t mRefCount;
    };
}
