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
    class Model
    {
    public:
        Model(scene::Camera* camera);
        ~Model();

        void SubmitCommand();
        void DrawVk(VkRenderer& renderer);
        void Update();

        void SetMesh(const Mesh& mesh);
        void SetHighlight(bool bSelection);

        int32_t  GetSubMeshCount() const;
        XMFLOAT3 GetCenterPoint(int32_t subMeshIndex) const;
        XMMATRIX GetMatrix() const;
    private:

        Mesh mMesh;

        XMMATRIX mMatWorld;
        XMMATRIX mMatRotation;
        XMMATRIX mMatScale;

        bool mbHighlight;
        bool mbActiveEmissive;
    };
}
