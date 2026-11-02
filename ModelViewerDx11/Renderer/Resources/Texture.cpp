#include <memory>

#include "Buffer.h"
#include "Image.h"
#include "Texture.h"
#include "../Uploader.h"
#include "../VkHelper.h"
#include "../../Util/Define.h"
#include "../../Util/Macro.h"
#include "../../Util/Util.h"

namespace renderer
{
    class VkHelper;

    Texture::Texture(const VkDevice& device)
        : mDevice(device)
        , mImage(nullptr)
        , mImageView(nullptr)
    {
    }

    Texture::~Texture()
    {
        delete mImage;
        vkDestroyImageView(mDevice, mImageView, nullptr);
    }

    bool Texture::Create(const int8_t* const filePath, bool bIsDDS)
    {
        ASSERT(filePath != nullptr, "invalid path. (filePath is nullptr) ");
        ASSERT(*filePath != ' ', "invalid path. (filePath may empty string) ");
        ASSERT(*filePath != '\0', "invalid path. (filePath may empty(null) string) ");


        DirectX::ScratchImage rawImage;
        if(loadImage(filePath, rawImage) == false)
        {
            ASSERT(false, "텍스처 로드 실패");
            return false;
        }

        const uint32_t graphicsQueueFamilyIndex = VkHelper::GetGraphicsQueueFamilyIndex();
        std::unique_ptr<Buffer> stagingBuffer = std::make_unique<Buffer>(mDevice);
        if(stagingBuffer->Create(rawImage.GetPixelsSize(), Buffer::eBufferType::Staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, graphicsQueueFamilyIndex) == false)
        {
            return false;
        }

        const uint32_t elementSize = rawImage.GetPixelsSize() / (rawImage.GetMetadata().height * rawImage.GetMetadata().width);
        stagingBuffer->AddDataOffset(rawImage.GetPixels(), rawImage.GetImageCount(), elementSize, 0);
        //
        // MEMO: 이미지 및 메모리 생성

        mImage = new Image(mDevice);

        VkExtent3D imageExtent{ static_cast<uint32_t>(rawImage.GetMetadata().width), static_cast<uint32_t>(rawImage.GetMetadata().height), 1 };
        mImage->Create(VK_IMAGE_LAYOUT_UNDEFINED, graphicsQueueFamilyIndex, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent, bIsDDS ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        //createImage(VK_IMAGE_LAYOUT_UNDEFINED, mGraphicsQueueIndex, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, { static_cast<uint32_t>(rawImage.GetMetadata().width), static_cast<uint32_t>(rawImage.GetMetadata().height), 1 }, bIsDDS ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, true, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newTexture.ImageMemory, newTexture.Image);

        // MEMO: staging buffer에서 이미지 메모리로 복사
        VkBufferImageCopy2 copyImageInfo2{};
        copyImageInfo2.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
        copyImageInfo2.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyImageInfo2.imageOffset = { 0, 0, 0 };
        copyImageInfo2.imageExtent = imageExtent;
        copyImageInfo2.bufferImageHeight = 0;
        copyImageInfo2.bufferRowLength = 0;
        Uploader::CopyBufferToImage(stagingBuffer->GetBuffer(), mImage->GetImage(), copyImageInfo2);


        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageViewCreateInfo.image = mImage->GetImage();
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        const VkResult result = vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mImageView);
        if(result != VK_SUCCESS)
        {
            return false;
        }

        return true;
    }

    void Texture::Release()
    {
        // MEMO: Pooling 시에 활용할 리셋 함수
    }

    bool Texture::loadImage(const int8_t* const filePath, DirectX::ScratchImage& outImage)
    {
        wchar_t texFilePath[util::MAX_PATH_LENGTH];
        const uint32_t filePathLength = strlen(reinterpret_cast<char const*>(filePath));
        size_t  numConverted = 0;
        (void)mbstowcs_s(&numConverted, texFilePath, util::MAX_PATH_LENGTH, reinterpret_cast<char const*>(filePath), filePathLength);
        ASSERT(numConverted == (filePathLength + 1), "conversion result is incorrect. converted(%u) pathLength(%u)", numConverted, filePathLength);

        const HRESULT result = LoadFromWICFile(texFilePath, DirectX::WIC_FLAGS_NONE, nullptr, outImage);
        if (FAILED(result))
        {
            return false;
        }

        return true;
    }
}
