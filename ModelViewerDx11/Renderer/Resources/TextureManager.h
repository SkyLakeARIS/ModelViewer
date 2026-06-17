#pragma once
#include "TextureData.h"
#include "../../framework.h"

namespace renderer
{
    class TextureManager
    {
    public:
        TextureManager(ID3D11Device* device);
        ~TextureManager();

        void AddTexture(const int8_t* const filePath, HashID& outTexHash);
        void AddDTextureDDS(const int8_t* const filePath, HashID& outTexHash);

        void RemoveTexture(HashID hash);

        ID3D11ShaderResourceView* GetTextureByHash(HashID hash);
    private:
        ID3D11Device* mDevice;
        std::unordered_map<HashID, TextureData> mTextures;
    };

}
