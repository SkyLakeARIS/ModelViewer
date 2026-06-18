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


    class Model
    {
    public:
        Model(scene::Camera* camera, const int8_t* const filePath);
        ~Model();

        void Draw();
        void DrawShadow();

        void Update();

        void SetMeshes(std::vector<Mesh>& meshes);
        void SetCenterPoint(XMFLOAT4& centerPoint);

        void SetHighlight(bool bSelection);

        XMFLOAT3 GetCenterPoint() const;

    private:

        HashID mModelHash;
        size_t mNumMesh;
        size_t mNumVertex;

        std::vector<Mesh> mMeshes;

        XMFLOAT3 mCenterPosition;
        XMMATRIX mMatWorld;
        XMMATRIX mMatRotation;
        XMMATRIX mMatScale;

        bool mbHighlight;
        bool mbActiveEmissive;
    };
}
