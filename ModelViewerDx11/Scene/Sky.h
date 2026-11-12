#pragma once
#include "../Renderer/Resources/Mesh.h"

namespace renderer
{
    class Renderer;
    class DescriptorSet;
}

namespace scene
{

    class Sky
    {
    public:

        Sky();
        ~Sky();

        void SubmitCommand(renderer::Renderer& renderer);
        void Update(const XMFLOAT3& cameraPosition);

        void SetMesh(const renderer::Mesh& mesh);
        void SetDescriptorSets(renderer::eShader shaderType, std::vector<renderer::DescriptorSet*>& descriptorSets);

    private:
        renderer::Mesh mMesh;
        // MEMO: Layout 구조 정비가 되기 전까지, 셰이더마다 SetLayout이 다르므로 분리, 같은 Layout이어도 Mesh마다 리소스가 다르므로 분리.
        std::unordered_map<renderer::eShader, std::vector<renderer::DescriptorSet*>> mDescriptorSets;

        XMMATRIX mWorld;
    };
}
