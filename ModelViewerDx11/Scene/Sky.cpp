#include "Sky.h"
#include "Camera.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Primitive/MeshGenerator.h"
#include "../Renderer/Resources/TextureManager.h"

namespace scene
{
    Sky::Sky(Camera& camera)
        : mCamera(&camera)
        , mMesh()
        , mWorld(XMMatrixIdentity())
    {
    }

    Sky::~Sky()
    {
        mCamera = nullptr;
        // TODO: 추가한 BufferData 처리할 수 있는 로직이 필요함. - (종료될 떄 처리되기 때문에 당장 문제는 없음)
    }

    HRESULT Sky::Initialize(uint32 latLines, uint32 lonLines, renderer::TextureManager* const texManager)
    {
 
        renderer::MeshGenerator::CreateSphere(latLines, lonLines, mMesh);

        const int8_t* const filePath = reinterpret_cast<const int8_t*>("./AssetData/textures/skybox.dds");
        texManager->AddDTextureDDS(filePath, mMesh.TextureHashes[static_cast<int8_t>(renderer::eTextureType::Diffuse)]);
        return S_OK;
    }

    void Sky::Draw(renderer::Renderer& renderer)
    {
        // render
        renderer.BindInputLayoutTo(renderer::eInputLayout::P);

        const int16_t strideVertex = renderer::GetVertexStrideSize(mMesh.VertexLayoutType);

        renderer.BindVertexBuffer(strideVertex);
        renderer.BindIndexBuffer();

        renderer.BindRasterStateByType(renderer::eRasterType::Skybox);

        renderer.BindShaderTo(renderer::eShader::Skybox);

        renderer.BindSamplerToPsByType(0, renderer::eSamplerType::AnisotropicWrap);


        renderer.BindCbToVsByType(0U, 1U, renderer::eCbType::CbWorld);
        renderer.BindCbToVsByType(1U, 1U, renderer::eCbType::CbViewProj);

        renderer.BindDepthStencilState(true);

        renderer.BindTextureToPs(0, mMesh.TextureHashes[static_cast<int8_t>(renderer::eTextureType::Diffuse)]);

        renderer.DrawIndexed(mMesh.IndexRange.Count, mMesh.IndexRange.StartIndex, mMesh.VertexRange.StartIndex);

        renderer.BindDepthStencilState(false);
    }

    void Sky::Update(renderer::Renderer& renderer)
    {
        // update
        XMFLOAT3 cameraPosition = mCamera->GetCameraPositionFloat();
        XMMATRIX matTranslate = XMMatrixIdentity();
        XMMATRIX matScale = XMMatrixScaling(100.0f, 100.0f, 100.0f);

        matTranslate = XMMatrixTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z);

        mWorld = matScale * matTranslate;

        renderer::CbWorld cbWVP;
        cbWVP.Matrix = XMMatrixTranspose(mWorld);
        renderer.UpdateCB(renderer::eCbType::CbWorld, &cbWVP);
    }
}
