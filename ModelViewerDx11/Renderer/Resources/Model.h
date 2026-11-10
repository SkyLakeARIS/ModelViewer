#pragma once
#include <unordered_map>

#include "../Resources/Mesh.h"

namespace renderer
{
    class DescriptorSet;
    class Renderer;
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
        Model();
        ~Model();

        void SubmitCommand();
        void Draw(Renderer& renderer);
        void DrawShadow(Renderer& renderer);
        void Update();

        void SetMesh(const Mesh& mesh);
        void SetHighlight(bool bSelection);

        void SetDescriptorSets(renderer::eShader shaderType, std::vector<DescriptorSet*>& descriptorSets);

        int32_t  GetSubMeshCount() const;
        XMFLOAT3 GetCenterPoint(int32_t subMeshIndex) const;

    private:

        Mesh mMesh;
        // MEMO: Layout 구조 정비가 되기 전까지, 셰이더마다 SetLayout이 다르므로 분리, 같은 Layout이어도 Mesh마다 리소스가 다르므로 분리.
        std::unordered_map<eShader, std::vector<DescriptorSet*>> mDescriptorSets;

        XMMATRIX mMatWorld;
        XMMATRIX mMatRotation;
        XMMATRIX mMatScale;

        bool mbHighlight;
        bool mbActiveEmissive;
    };
}
