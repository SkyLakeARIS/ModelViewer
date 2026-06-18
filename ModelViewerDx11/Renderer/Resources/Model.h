#pragma once
#include "../Resources/ModelData.h"

namespace scene
{
    class Light;
    class Camera;
}

namespace renderer
{
    class ModelImporter;

    // TODO: cleanup - RenderTypes 헤더로 이동
    typedef Material CbMaterial;

    class Model
    {
    public:
        Model(scene::Camera* camera, const int8_t* const filePath);
        ~Model();

        void DrawNew();
        void DrawShadowNew();

        void Update();

        void SetMeshes(std::vector<MeshNew>& meshes);
        void SetCenterPoint(XMFLOAT4& centerPoint);

        void SetHighlight(bool bSelection);

        XMFLOAT3 GetCenterPoint() const;

    private:

        HashID mModelHash;
        size_t mNumMesh;
        size_t mNumVertex;

        std::vector<MeshNew> mMeshesNew;

        XMFLOAT3 mCenterPosition;
        XMMATRIX mMatWorld;
        XMMATRIX mMatRotation;
        XMMATRIX mMatScale;

        bool mbHighlight;
        bool mbActiveEmissive;
    };
}
