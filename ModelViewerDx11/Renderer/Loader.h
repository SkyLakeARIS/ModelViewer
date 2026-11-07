#pragma once
#include "../framework.h"
#include "../Util/Type.h"

namespace scene
{
    class Billboard;
    class Sky;
}

namespace renderer
{
    struct Mesh;
    class Model;
    class ModelImporter;
    class Renderer;
    class DescriptorAllocator;
    class ResourceManager;

    class Loader
    {
    public:
        Loader(ModelImporter* const importer, Renderer* const renderer, DescriptorAllocator* const descriptorAllocator);
        ~Loader();

        bool Load(const int8_t* const filePath, Model** outModel) const;
        // TODO: 포팅 작업을 완료하고 고도화 작업을 위해서 우선은 기존 Scene을 구성하던 물체 개별 로드 함수를 생성 - 고도화 시 자체 포멧에 따라 일반화 예정
        bool LoadSkybox(scene::Sky* const sky);
        bool LoadBillboard(scene::Billboard* const billboard);
    private:
        bool loadTexture(const int8_t* const filePath, HashID texHash, bool bIsDDS) const;

    private:
        ModelImporter* mImporter;
        Renderer* mRenderer;
        ResourceManager* mResourceManager;
        DescriptorAllocator* mDescriptorAllocator;
    };
}
