#pragma once
#include "../Resources/ModelData.h"

namespace scene
{
    class Camera;
}

namespace renderer
{
    class BufferManager;
    class Renderer;


    class Model
    {
    public:
        Model(scene::Camera* camera, BufferManager* bufferManager);
        ~Model();

        void Draw(renderer::Renderer& renderer);
        void DrawShadow(renderer::Renderer& renderer);

        void Update(renderer::Renderer& renderer);

        void SetMeshes(std::vector<Mesh>& meshes);
        void SetCenterPoint(XMFLOAT4& centerPoint);

        void SetHighlight(bool bSelection);

        XMFLOAT3 GetCenterPoint() const;

    private:
        BufferManager* mBufferManager;

        std::vector<Mesh> mMeshes;

        XMFLOAT3 mCenterPosition;
        XMMATRIX mMatWorld;
        XMMATRIX mMatRotation;
        XMMATRIX mMatScale;

        bool mbHighlight;
        bool mbActiveEmissive;
    };
}
