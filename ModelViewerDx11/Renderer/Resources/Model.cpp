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
        for(auto& mesh : mMeshes)
        {
            const int16_t stride = GetVertexStrideSize(mesh.VertexLayoutType);
            const int16_t strideIndex = mBufferManager->GetIndexStrideSize();
            mBufferManager->RemoveVertexData(stride, mesh.MeshHash);
            mBufferManager->RemoveIndexData(strideIndex, mesh.MeshHash);
        }
        mBufferManager = nullptr;
    }

    void Model::Draw(renderer::Renderer& renderer)
    {
        renderer.BindInputLayoutTo(eInputLayout::PTN);

        const uint32 stride = sizeof(VertexPTN);

        renderer.BindVertexBufferNew(stride, 0);
        renderer.BindIndexBufferNew(0);

        // outline

        if (mbHighlight)
        {
            renderer.BindRasterStateByType(eRasterType::CullBack);

            renderer.BindShaderTo(eShader::Outline);
            renderer.BindCbToVsByType(0U, 1U, eCbType::CbWorld);
            renderer.BindCbToVsByType(1U, 1U, eCbType::CbOutlineProperty);
            renderer.BindCbToVsByType(2U, 1U, eCbType::CbViewProj);

            for (uint32_t index = 0U; index < mMeshes.size(); ++index)
            {
                renderer.DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexRange.Count), mMeshes[index].IndexRange.StartIndex, mMeshes[index].VertexRange.StartIndex);
            }
            // reset for basic draw
            renderer.ClearDepthBuffer();
        }

        renderer.BindRasterStateByType(eRasterType::Basic);

        renderer.BindShaderTo(eShader::BasicWithShadow);

        renderer.BindCbToVsByType(0U, 1U, eCbType::CbWorld);
        renderer.BindCbToVsByType(1U, 1U, eCbType::CbLightViewProjMatrix);
        renderer.BindCbToVsByType(2U, 1U, eCbType::CbLightProperty);
        renderer.BindCbToVsByType(3U, 1U, eCbType::CbCameraPosition);
        renderer.BindCbToVsByType(4U, 1U, eCbType::CbViewProj);

        renderer.BindSamplerToPsByType(0, eSamplerType::AnisotropicWrap);

        renderer.BindCbToPs(0U, 1U, eCbType::CbMaterial);

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
            renderer.UpdateCB(eCbType::CbMaterial, &cbMaterial);

            renderer.DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexRange.Count), mMeshes[index].IndexRange.StartIndex, mMeshes[index].VertexRange.StartIndex);
        }

        renderer.UnbindTexturePs(2);
    }

    void Model::DrawShadow(renderer::Renderer& renderer)
    {
        renderer.BindInputLayoutTo(eInputLayout::P);

        const uint32 stride = sizeof(VertexPTN);

        renderer.BindVertexBufferNew(stride, 0);
        renderer.BindIndexBufferNew(0);

        renderer.BindRasterStateByType(eRasterType::Outline);
        renderer.BindShaderTo(eShader::Shadow);

        renderer.BindCbToVsByType(0U, 1U, eCbType::CbWorld);
        renderer.BindCbToVsByType(1U, 1U, eCbType::CbLightViewProjMatrix);

        // Draw
        for (size_t index = 0U; index < mMeshes.size(); ++index)
        {
            renderer.DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexRange.Count), mMeshes[index].IndexRange.StartIndex, mMeshes[index].VertexRange.StartIndex);
        }
    }

    void Model::Update(renderer::Renderer& renderer)
    {
        CbWorld cbWorld;
        cbWorld.Matrix = XMMatrixTranspose(mMatWorld);

        renderer.UpdateCB(eCbType::CbWorld, &cbWorld);
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
