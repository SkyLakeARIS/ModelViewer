#include "ResourceManager.h"
#include "Texture.h"

namespace renderer
{
    ResourceManager::~ResourceManager()
    {
        for(auto& texIt : mTextures)
        {
            delete texIt.second;
            texIt.second = nullptr;
        }
        mTextures.clear();
    }

    bool ResourceManager::TryAddTexture(HashID hash, Texture* const texture)
    {
        if(mTextures.find(hash) != mTextures.end())
        {
            return false;
        }

        mTextures.insert({ hash, texture });
        return true;
    }

    Texture* ResourceManager::GetTexture(HashID hash)
    {
        Texture* tex = nullptr;
        const auto& texIt = mTextures.find(hash);
        if (texIt != mTextures.end())
        {
            tex = texIt->second;
        }
        return tex;
    }
}
