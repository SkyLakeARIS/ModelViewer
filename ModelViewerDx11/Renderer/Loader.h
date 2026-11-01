#pragma once
#include "../framework.h"

namespace renderer
{
    class Model;
    class ModelImporter;
    class Renderer;
    class DescriptorAllocator;
    class ResourceManager;

    class Loader
    {
    public:
        Loader(ModelImporter* const importer, Renderer* const renderer, ResourceManager* const resourceManager, DescriptorAllocator* const descriptorAllocator);
        ~Loader();

        bool Load(const int8_t* const filePath, Model** outModel) const;
    private:
        ModelImporter* mImporter;
        Renderer* mRenderer;
        ResourceManager* mResourceManager;
        DescriptorAllocator* mDescriptorAllocator;
    };
}
