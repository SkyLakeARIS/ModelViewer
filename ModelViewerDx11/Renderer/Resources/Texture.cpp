#include <memory>
#include "Buffer.h"
#include "Image.h"
#include "Texture.h"
#include "../Uploader.h"
#include "../VkHelper.h"
#include "../../Util/Define.h"
#include "../../Util/Macro.h"

namespace renderer
{

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
        if (loadImage(filePath, rawImage, bIsDDS) == false)
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

        stagingBuffer->AddDataOffset(rawImage.GetPixels(), rawImage.GetPixelsSize(), 1, 0);
        //
        // MEMO: 이미지 및 메모리 생성

        mImage = new Image(mDevice);

        const uint32_t imageCount = rawImage.GetImageCount();
        VkExtent3D imageExtent{ static_cast<uint32_t>(rawImage.GetMetadata().width), static_cast<uint32_t>(rawImage.GetMetadata().height), 1 };
        const VkImageCreateFlags createFlags = bIsDDS ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
        mImage->Create(VK_IMAGE_LAYOUT_UNDEFINED, graphicsQueueFamilyIndex, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent, imageCount, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, createFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // MEMO: staging buffer에서 이미지 메모리로 복사
        std::vector<VkBufferImageCopy2> regions;
        for (uint32_t nowImage = 0; nowImage < imageCount; ++nowImage)
        {
            VkBufferImageCopy2 copyImageInfo2{};
            copyImageInfo2.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
            VkImageSubresourceLayers subresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, nowImage, 1};
            copyImageInfo2.imageSubresource = subresourceLayers;
            copyImageInfo2.imageOffset = { 0, 0, 0 };
            copyImageInfo2.imageExtent = imageExtent;
            copyImageInfo2.bufferOffset = rawImage.GetImage(0, nowImage, 0)->pixels - rawImage.GetPixels();
            copyImageInfo2.bufferImageHeight = 0;
            copyImageInfo2.bufferRowLength = 0;
            regions.push_back(copyImageInfo2);
        }

        Uploader::CopyBufferToImage(stagingBuffer->GetBuffer(), mImage->GetImage(), regions);


        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageViewCreateInfo.image = mImage->GetImage();
        imageViewCreateInfo.viewType = bIsDDS ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
        VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        subresourceRange.layerCount = imageCount;
        imageViewCreateInfo.subresourceRange = subresourceRange;
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

    VkImageView Texture::GetImageView() const
    {
        return mImageView;
    }

    bool Texture::loadImage(const int8_t* const filePath, DirectX::ScratchImage& outImage, bool bIsDDS)
    {
        wchar_t texFilePath[util::MAX_PATH_LENGTH];
        const uint32_t filePathLength = strlen(reinterpret_cast<char const*>(filePath));
        size_t  numConverted = 0;
        (void)mbstowcs_s(&numConverted, texFilePath, util::MAX_PATH_LENGTH, reinterpret_cast<char const*>(filePath), filePathLength);
        ASSERT(numConverted == (filePathLength + 1), "conversion result is incorrect. converted(%u) pathLength(%u)", numConverted, filePathLength);

        HRESULT result;
        if(bIsDDS)
        {
            result = LoadFromDDSFile(texFilePath, DirectX::DDS_FLAGS_FORCE_RGB, nullptr, outImage);
        }
        else
        {
            result = LoadFromWICFile(texFilePath, DirectX::WIC_FLAGS_FORCE_RGB, nullptr, outImage);
        }

        if (FAILED(result))
        {
            return false;
        }

        return true;
    }
}
