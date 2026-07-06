#pragma once
#include "../../framework.h"

namespace renderer
{
    class Model;
    class BufferManager;
    class ModelImporter;
    class TextureManager;

    class ResourceManager
    {
    public:
        ResourceManager(ID3D11Device* const device, TextureManager* const textureManager, ModelImporter* const importer, BufferManager* const bufferManager);
        ~ResourceManager();

        void LoadModel(const int8_t* const filePath, Model* const outModel);
    private:
        ID3D11Device* mDevice;
        TextureManager* mTextureManager;
        ModelImporter* mModelImporter;
        BufferManager* mBufferManager;
    };
}
