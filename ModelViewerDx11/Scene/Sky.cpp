#include "Sky.h"
#include "../Renderer/Primitive/MeshGenerator.h"

namespace scene
{
    Sky::Sky()
        : mMesh()
        , mWorld(XMMatrixIdentity())
    {
    }

    Sky::~Sky()
    {}

    bool Sky::Initialize(uint32_t latLines, uint32_t lonLines, renderer::TextureManager* const texManager)
    {
        renderer::MeshGenerator::CreateSphere(latLines, lonLines, mMesh);
        mMesh.SubMeshes.front().Material.Factors.IsLitOn = static_cast<float>(false);

        return true;
    }

    void Sky::SubmitCommand()
    {

    }

    void Sky::Update(const XMFLOAT3& cameraPosition)
    {
        // update
        constexpr float SKY_SCALE_SIZE = 100.0f;
        const XMMATRIX matScale = XMMatrixScaling(SKY_SCALE_SIZE, SKY_SCALE_SIZE, SKY_SCALE_SIZE);
        const XMMATRIX matTranslate = XMMatrixTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z);

        mWorld = XMMatrixTranspose(matScale * matTranslate);

    }
}
