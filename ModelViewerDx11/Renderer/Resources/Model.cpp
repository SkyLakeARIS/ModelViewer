#include "Model.h"
#include "BufferManager.h"
#include "../Renderer.h"
#include "../../Util/Macro.h"
#include "../../Util/Util.h"
#include "../Importer/ModelImporter.h"

namespace renderer
{
    Model::Model(scene::Camera* camera, const int8_t* const filePath, BufferManager* bufferManager)
        : mBufferManager(bufferManager)
        , mModelHash(0)
        , mCenterPosition(0.0f, 0.0f, 0.0f)
        , mMatRotation(XMMatrixIdentity())
        , mMatScale(XMMatrixIdentity())
        , mbHighlight(false)
        , mbActiveEmissive(false)
    {
        ASSERT(camera != nullptr, "do not pass nullptr");

        mMatWorld  = XMMatrixIdentity();
        mModelHash = util::GetDjb2Hash(filePath);
    }

    Model::~Model()
    {
        mBufferManager->RemoveVertexData(mModelHash);
        mBufferManager->RemoveIndexData(mModelHash);
        mBufferManager = nullptr;
    }

    void Model::Draw(renderer::Renderer& renderer)
    {
        renderer.BindInputLayoutTo(Renderer::eInputLayout::PTN);

        const uint32 stride = sizeof(Vertex);

        renderer.BindVertexBuffer(stride, 0);
        renderer.BindIndexBuffer(0);

        // outline

        if (mbHighlight)
        {
            renderer.BindRasterStateByType(Renderer::eRasterType::CullBack);

            renderer.BindShaderTo(Renderer::eShader::Outline);
            renderer.BindCbToVsByType(0U, 1U, Renderer::eCbType::CbWorld);
            renderer.BindCbToVsByType(1U, 1U, Renderer::eCbType::CbOutlineProperty);
            renderer.BindCbToVsByType(2U, 1U, Renderer::eCbType::CbViewProj);

            for (uint32_t index = 0U; index < mMeshes.size(); ++index)
            {
                renderer.DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexRange.Count), mMeshes[index].IndexRange.StartIndex, mMeshes[index].VertexRange.StartIndex);
            }
            // reset for basic draw
            renderer.ClearDepthBuffer();
        }

        renderer.BindRasterStateByType(Renderer::eRasterType::Basic);

        renderer.BindShaderTo(Renderer::eShader::BasicWithShadow);

        renderer.BindCbToVsByType(0U, 1U, Renderer::eCbType::CbWorld);
        renderer.BindCbToVsByType(1U, 1U, Renderer::eCbType::CbLightViewProjMatrix);
        renderer.BindCbToVsByType(2U, 1U, Renderer::eCbType::CbLightProperty);
        renderer.BindCbToVsByType(3U, 1U, Renderer::eCbType::CbCameraPosition);
        renderer.BindCbToVsByType(4U, 1U, Renderer::eCbType::CbViewProj);

        renderer.BindSamplerToPsByType(0, Renderer::eSamplerType::AnisotropicWrap);

        renderer.BindCbToPs(0U, 1U, Renderer::eCbType::CbMaterial);

        renderer.BindShadowTextureToPs(2);

        // Draw
        for (size_t index = 0U; index < mMeshes.size(); ++index)
        {
            renderer.BindTextureToPs(0, mMeshes[index].TextureHashes[static_cast<int8_t>(eTextureType::Diffuse)]);
            if(mMeshes[index].TextureHashes[static_cast<int8_t>(eTextureType::Normal)])
            {
                renderer.BindTextureToPs(1, mMeshes[index].TextureHashes[static_cast<int8_t>(eTextureType::Normal)]);
            }
            CbMaterial cbMaterial;
            ZeroMemory(&cbMaterial, sizeof(CbMaterial));

            memcpy(&cbMaterial, &mMeshes[index].Material, sizeof(Material));
            if (!mbActiveEmissive)
            {
                cbMaterial.Emissive = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }
            renderer.UpdateCB(Renderer::eCbType::CbMaterial, &cbMaterial);

            renderer.DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexRange.Count), mMeshes[index].IndexRange.StartIndex, mMeshes[index].VertexRange.StartIndex);
        }

        renderer.UnbindTexturePs(2);
    }

    void Model::DrawShadow(renderer::Renderer& renderer)
    {
        renderer.BindInputLayoutTo(Renderer::eInputLayout::P);

        const uint32 stride = sizeof(Vertex);

        renderer.BindVertexBuffer(stride, 0);
        renderer.BindIndexBuffer(0);

        renderer.BindRasterStateByType(Renderer::eRasterType::Outline);
        renderer.BindShaderTo(Renderer::eShader::Shadow);

        renderer.BindCbToVsByType(0U, 1U, Renderer::eCbType::CbWorld);
        renderer.BindCbToVsByType(1U, 1U, Renderer::eCbType::CbLightViewProjMatrix);

        // Draw
        for (size_t index = 0U; index < mMeshes.size(); ++index)
        {
            renderer.DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexRange.Count), mMeshes[index].IndexRange.StartIndex, mMeshes[index].VertexRange.StartIndex);
        }
    }

    void Model::Update(renderer::Renderer& renderer)
    {
        Renderer::CbWorld cbWorld;
        cbWorld.Matrix = XMMatrixTranspose(mMatWorld);

        renderer.UpdateCB(Renderer::eCbType::CbWorld, &cbWorld);
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
