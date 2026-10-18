#pragma once
#include "../Resources/Mesh.h"

namespace renderer
{
    class VkRenderer;
}

namespace scene
{
    class Camera;
}

namespace renderer
{
    struct RenderPacket;
    class BufferManager;


    class Model
    {
    public:
        Model(scene::Camera* camera, BufferManager* bufferManager);
        ~Model();

        void SubmitCommand(std::vector<renderer::RenderPacket>& commandList);
        void DrawVk(VkRenderer& renderer);
        void Update();

        void SetMesh(const Mesh& mesh);
        void SetHighlight(bool bSelection);

        int32_t  GetSubMeshCount() const;
        XMFLOAT3 GetCenterPoint(int32_t subMeshIndex) const;
        XMMATRIX GetMatrix() const;
    private:
        BufferManager* mBufferManager;

        Mesh mMesh;

        XMMATRIX mMatWorld;
        XMMATRIX mMatRotation;
        XMMATRIX mMatScale;

        bool mbHighlight;
        bool mbActiveEmissive;
    };
}
