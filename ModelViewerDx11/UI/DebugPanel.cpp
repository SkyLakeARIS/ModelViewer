#include "DebugPanel.h"
#include "../Renderer/Primitive/MeshGenerator.h"

namespace ui
{
    DebugPanel::DebugPanel(int16_t originX, int16_t originY, int16_t width, int16_t height)
    {
        renderer::MeshGenerator::CreateScreenPlane(originX, originY, width, height, mMesh);
        mMesh.SubMeshes.front().Material.Factors.IsLitOn = static_cast<float>(false);
    }

    DebugPanel::~DebugPanel()
    {
        
    }

    void DebugPanel::SubmitCommand()
    {

    }

    void DebugPanel::SetDebugType(HashID texHash, int16_t serialID)
    {
        // MEMO: 단일 메시이므로 즉시 접근
        renderer::SemiMaterial& material = mMesh.SubMeshes.front().Material;
        material.TextureHashes[static_cast<uint8_t>(renderer::eTextureType::Diffuse)] = texHash;
        material.TextureSerials[static_cast<uint8_t>(renderer::eTextureType::Diffuse)] = serialID;
    }
}
