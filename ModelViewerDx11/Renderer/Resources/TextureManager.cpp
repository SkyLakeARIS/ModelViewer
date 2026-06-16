#include "TextureManager.h"
#include "../../Util/Util.h"
#include "../../Util/Macro.h"
#include "../../Util/Define.h"

namespace renderer
{

    TextureManager::TextureManager(ID3D11Device* device)
        : mDevice(device)
    {
        ASSERT(device, "invalid device. device is nullptr.");
        mTextures.reserve(64);
    }

    TextureManager::~TextureManager()
    {
        for (auto& texIt: mTextures)
        {
            SAFETY_RELEASE(texIt.second.SRV);
        }
        mTextures.clear();
        mDevice = nullptr;
    }

    void TextureManager::AddTexture(const int8_t* const filePath, HashID& outTexHash)
    {
        ASSERT(filePath != nullptr, "invalid path. (filePath is nullptr) ");
        ASSERT(*filePath != ' ', "invalid path. (filePath may empty string) ");
        ASSERT(*filePath != '\0', "invalid path. (filePath may empty(null) string) ");

        outTexHash = 0;
        const HashID texHash = util::GetDjb2Hash(filePath);
        if(mTextures.find(texHash) == mTextures.end())
        {
            // TODO: improve - io는 나중에 분리하도록 하자.
            wchar_t texFilePath[util::MAX_PATH_LENGTH];
            const uint32_t filePathLength = strlen(reinterpret_cast<char const*>(filePath));
            size_t  numConverted = 0;
            (void)mbstowcs_s(&numConverted, texFilePath, util::MAX_PATH_LENGTH, reinterpret_cast<char const*>(filePath), filePathLength);
            ASSERT(numConverted == (filePathLength + 1), "conversion result is incorrect. converted(%u) pathLength(%u)", numConverted, filePathLength);

            ScratchImage rawImage;
            if(FAILED(LoadFromWICFile(texFilePath, WIC_FLAGS_NONE, nullptr, rawImage)))
            {
                return;
            }

            ID3D11Resource* tex = nullptr;
            if (FAILED(CreateTexture(mDevice, rawImage.GetImages(), rawImage.GetImageCount(), rawImage.GetMetadata(), &tex)))
            {
                rawImage.Release();
                return;
            }

            ID3D11ShaderResourceView* srv = nullptr;
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipLevels = 1;
            desc.Texture2D.MostDetailedMip = 0;
            if (FAILED(mDevice->CreateShaderResourceView(tex, &desc, &srv)))
            {
                ASSERT(false, "ShaderResourceView 생성 실패");
                SAFETY_RELEASE(tex);
                rawImage.Release();
                return;
            }
            SAFETY_RELEASE(tex);
            rawImage.Release();

            TextureData newTexData = {};
            newTexData.Hash = texHash;
            newTexData.SRV = srv;
            mTextures.insert(std::make_pair(texHash, newTexData));
            outTexHash = texHash;
        }
        else
        {
            outTexHash = texHash;
        }
    }

    void TextureManager::AddDTextureDDS(const int8_t* const filePath, HashID& outTexHash)
    {
        outTexHash = 0;

        wchar_t texFilePath[util::MAX_PATH_LENGTH];
        const uint32_t filePathLength = strlen(reinterpret_cast<char const*>(filePath));
        size_t  numConverted = 0;
        (void)mbstowcs_s(&numConverted, texFilePath, util::MAX_PATH_LENGTH, reinterpret_cast<char const*>(filePath), filePathLength);
        ASSERT(numConverted == (filePathLength + 1), "conversion result is incorrect. converted(%u) pathLength(%u)", numConverted, filePathLength);

        ScratchImage image;
        if (FAILED(LoadFromDDSFile(texFilePath, DDS_FLAGS_NONE, nullptr, image)))
        {
            ASSERT(false, "failed to load DDS file : DDS 로드 실패");
            return;
        }

        ID3D11Texture2D* textureResource = nullptr;
        if (FAILED(CreateTexture(mDevice, image.GetImages(), image.GetImageCount(), image.GetMetadata(), (ID3D11Resource**)&textureResource)))
        {
            image.Release();
            ASSERT(false, "failed to create TextureResource : gTextureResource 생성 실패");
            return;
        }
        D3D11_TEXTURE2D_DESC desc;
        textureResource->GetDesc(&desc);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = 1;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.Format = desc.Format;
        srvDesc.TextureCube.MipLevels = desc.MipLevels;

        ID3D11ShaderResourceView* srv = nullptr;
        if (FAILED(mDevice->CreateShaderResourceView(textureResource, &srvDesc, &srv)))
        {
            ASSERT(false, "failed to create outShaderResourceView : outShaderResourceView 생성 실패");
            image.Release();
            SAFETY_RELEASE(textureResource);
            return;
        }

        const HashID texHash = util::GetDjb2Hash(filePath);
        TextureData newTexData = {};
        newTexData.Hash = texHash;
        newTexData.SRV = srv;
        mTextures.insert(std::make_pair(texHash, newTexData));
        outTexHash = texHash;

        image.Release();
        SAFETY_RELEASE(textureResource);
    }

    void TextureManager::RemoveTexture(HashID hash)
    {
        ASSERT(hash > 0, "Hash is invalid. may not initialized. Hash(%d)", hash);
        // TODO: RefCount가 필요할까? - 잊지 않기 위해 TODO로 설정
        mTextures.erase(hash);
    }

    ID3D11ShaderResourceView* TextureManager::GetTextureByHash(HashID hash)
    {
        ASSERT(hash > 0, "Hash is invalid. Hash(%d)", hash);
        const auto& texIt = mTextures.find(hash);
        if(texIt == mTextures.end())
        {
            return nullptr;
        }
        return texIt->second.SRV;
    }
}
