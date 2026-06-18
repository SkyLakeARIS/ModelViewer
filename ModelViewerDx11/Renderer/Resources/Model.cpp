#include "Model.h"
#include "BufferManager.h"
#include "../Renderer.h"
#include "../../Util/Macro.h"
#include "../../Util/Util.h"
#include "../Importer/ModelImporter.h"

namespace renderer
{
    Model::Model(scene::Camera* camera, const int8_t* const filePath)
        : mModelHash(0)
        , mCenterPosition(0.0f, 0.0f, 0.0f)
        , mMatRotation(XMMatrixIdentity())
        , mMatScale(XMMatrixIdentity())
        , mbHighlight(false)
        , mbActiveEmissive(false)
    {
        ASSERT(camera != nullptr, "do not pass nullptr");

        mMatWorld = XMMatrixIdentity();
        mModelHash = util::GetDjb2Hash(filePath);
    }

    Model::~Model()
    {
        BufferManager* const bufferManager = Renderer::GetInstance()->GetBufferManager();
        bufferManager->RemoveVertexData(mModelHash);
        bufferManager->RemoveIndexData(mModelHash);
    }

    void Model::Draw()
    {
        Renderer::GetInstance()->BindInputLayoutTo(Renderer::eInputLayout::PTN);

        const uint32 stride = sizeof(Vertex);

        Renderer::GetInstance()->BindVertexBuffer(stride, 0);
        Renderer::GetInstance()->BindIndexBuffer(0);

        // outline
        ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();

        if (mbHighlight)
        {
            Renderer::GetInstance()->BindRasterStateByType(Renderer::eRasterType::CullBack);

            Renderer::GetInstance()->BindShaderTo(Renderer::eShader::Outline);
            Renderer::GetInstance()->BindCbToVsByType(0U, 1U, Renderer::eCbType::CbWorld);
            Renderer::GetInstance()->BindCbToVsByType(1U, 1U, Renderer::eCbType::CbOutlineProperty);
            Renderer::GetInstance()->BindCbToVsByType(2U, 1U, Renderer::eCbType::CbViewProj);

            for (uint32_t index = 0U; index < mMeshes.size(); ++index)
            {
                deviceContext->DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexRange.Count), mMeshes[index].IndexRange.StartIndex, mMeshes[index].VertexRange.StartIndex);
            }
            // reset for basic draw
            Renderer::GetInstance()->ClearDepthBuffer();
        }

        Renderer::GetInstance()->BindRasterStateByType(Renderer::eRasterType::Basic);

        Renderer::GetInstance()->BindShaderTo(Renderer::eShader::BasicWithShadow);

        Renderer::GetInstance()->BindCbToVsByType(0U, 1U, Renderer::eCbType::CbWorld);
        Renderer::GetInstance()->BindCbToVsByType(1U, 1U, Renderer::eCbType::CbLightViewProjMatrix);
        Renderer::GetInstance()->BindCbToVsByType(2U, 1U, Renderer::eCbType::CbLightProperty);
        Renderer::GetInstance()->BindCbToVsByType(3U, 1U, Renderer::eCbType::CbCameraPosition);
        Renderer::GetInstance()->BindCbToVsByType(4U, 1U, Renderer::eCbType::CbViewProj);

        Renderer::GetInstance()->BindSamplerToPsByType(0, Renderer::eSamplerType::AnisotropicWrap);

        Renderer::GetInstance()->BindCbToPs(0U, 1U, Renderer::eCbType::CbMaterial);

        Renderer::GetInstance()->BindShadowTextureToPs(2);

        // Draw
        for (size_t index = 0U; index < mMeshes.size(); ++index)
        {
            Renderer::GetInstance()->BindTextureToPs(0, mMeshes[index].TextureHashes[static_cast<int8_t>(eTextureType::Diffuse)]);
            if(mMeshes[index].TextureHashes[static_cast<int8_t>(eTextureType::Normal)])
            {
                Renderer::GetInstance()->BindTextureToPs(1, mMeshes[index].TextureHashes[static_cast<int8_t>(eTextureType::Normal)]);
            }
            CbMaterial cbMaterial;
            ZeroMemory(&cbMaterial, sizeof(CbMaterial));

            memcpy(&cbMaterial, &mMeshes[index].Material, sizeof(Material));
            if (!mbActiveEmissive)
            {
                cbMaterial.Emissive = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }
            Renderer::GetInstance()->UpdateCB(Renderer::eCbType::CbMaterial, &cbMaterial);

            deviceContext->DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexRange.Count), mMeshes[index].IndexRange.StartIndex, mMeshes[index].VertexRange.StartIndex);
        }

        Renderer::GetInstance()->UnbindTexturePs(2);
    }

    void Model::DrawShadow()
    {
        Renderer::GetInstance()->BindInputLayoutTo(Renderer::eInputLayout::P);

        const uint32 stride = sizeof(Vertex);

        Renderer::GetInstance()->BindVertexBuffer(stride, 0);
        Renderer::GetInstance()->BindIndexBuffer(0);

        Renderer::GetInstance()->BindRasterStateByType(Renderer::eRasterType::Outline);
        Renderer::GetInstance()->BindShaderTo(Renderer::eShader::Shadow);

        Renderer::GetInstance()->BindCbToVsByType(0U, 1U, Renderer::eCbType::CbWorld);
        Renderer::GetInstance()->BindCbToVsByType(1U, 1U, Renderer::eCbType::CbLightViewProjMatrix);

        // Draw
        ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();
        for (size_t index = 0U; index < mMeshes.size(); ++index)
        {
            deviceContext->DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexRange.Count), mMeshes[index].IndexRange.StartIndex, mMeshes[index].VertexRange.StartIndex);
        }
    }

    void Model::Update()
    {
        Renderer::CbWorld cbWorld;
        cbWorld.Matrix = XMMatrixTranspose(mMatWorld);

        Renderer::GetInstance()->UpdateCB(Renderer::eCbType::CbWorld, &cbWorld);
    }

    void Model::SetMeshes(std::vector<Mesh>& meshes)
    {
        ASSERT(mMeshes.empty(), "mMeshes is not empty.");
        mMeshes.swap(meshes);
    }

    void Model::SetCenterPoint(XMFLOAT4& centerPoint)
    {
        mCenterPosition = XMFLOAT3(centerPoint.x, centerPoint.y, centerPoint.z);
    }

    void Model::SetHighlight(bool bSelection)
    {
        mbHighlight = bSelection;
    }

    XMFLOAT3 Model::GetCenterPoint() const
    {
        XMFLOAT3 pos = mCenterPosition;
        pos.y += 1.0f;
        return pos;
    }
}
