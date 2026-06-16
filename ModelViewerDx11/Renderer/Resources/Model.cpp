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
        renderer::BufferManager* const bufferManager = renderer::Renderer::GetInstance()->GetBufferManager();
        bufferManager->RemoveVertexData(mModelHash);
        bufferManager->RemoveIndexData(mModelHash);









    }

    void Model::DrawNew()
    {
        Renderer::GetInstance()->SetInputLayoutTo(Renderer::eInputLayout::PTN);

        BufferManager* const bufferManager = Renderer::GetInstance()->GetBufferManager();
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        const uint32 stride = sizeof(renderer::Vertex);

        Renderer::GetInstance()->BindVertexBuffer(stride, 0);
        Renderer::GetInstance()->BindIndexBuffer(0);

        // outline
        ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();

        if (mbHighlight)
        {
            Renderer::GetInstance()->SetRasterState(Renderer::eRasterType::CullBack);

            Renderer::GetInstance()->SetShaderTo(Renderer::eShader::Outline);
            Renderer::GetInstance()->BindCbToVsByType(0U, 1U, Renderer::eCbType::CbWorld);
            Renderer::GetInstance()->BindCbToVsByType(1U, 1U, Renderer::eCbType::CbOutlineProperty);
            Renderer::GetInstance()->BindCbToVsByType(2U, 1U, Renderer::eCbType::CbViewProj);

            for (uint32_t index = 0U; index < mMeshesNew.size(); ++index)
            {
                deviceContext->DrawIndexed(static_cast<uint32_t>(mMeshesNew[index].IndexRange.Count), mMeshesNew[index].IndexRange.StartIndex, mMeshesNew[index].VertexRange.StartIndex);
            }
            // reset for basic draw
            Renderer::GetInstance()->ClearDepthBuffer();
        }

        Renderer::GetInstance()->SetRasterState(Renderer::eRasterType::Basic);

        Renderer::GetInstance()->SetShaderTo(Renderer::eShader::BasicWithShadow);

        Renderer::GetInstance()->BindCbToVsByType(0U, 1U, Renderer::eCbType::CbWorld);
        Renderer::GetInstance()->BindCbToVsByType(1U, 1U, Renderer::eCbType::CbLightViewProjMatrix);
        Renderer::GetInstance()->BindCbToVsByType(2U, 1U, Renderer::eCbType::CbLightProperty);
        Renderer::GetInstance()->BindCbToVsByType(3U, 1U, Renderer::eCbType::CbCameraPosition);
        Renderer::GetInstance()->BindCbToVsByType(4U, 1U, Renderer::eCbType::CbViewProj);

        Renderer::GetInstance()->BindSamplerToPsByType(0, renderer::Renderer::eSamplerType::AnisotropicWrap);

        Renderer::GetInstance()->BindCbToPs(0U, 1U, Renderer::eCbType::CbMaterial);

        Renderer::GetInstance()->BindShadowTextureToPs(2);

        // Draw
        for (size_t index = 0U; index < mMeshesNew.size(); ++index)
        {
            // TODO: Bind 할 수 있도록 Renderer에 추가 필요함. Hash/Slot 전달하면 렌더러가 TextureManager에서 찾아서 바인드하도록.
            Renderer::GetInstance()->BindTextureToPs(0, mMeshesNew[index].TextureHashes[static_cast<int8_t>(eTextureType::Diffuse)]);
            if(mMeshesNew[index].TextureHashes[static_cast<int8_t>(eTextureType::Normal)])
            {
                Renderer::GetInstance()->BindTextureToPs(1, mMeshesNew[index].TextureHashes[static_cast<int8_t>(eTextureType::Normal)]);
            }
            CbMaterial cbMaterial;
            ZeroMemory(&cbMaterial, sizeof(CbMaterial));

            memcpy(&cbMaterial, &mMeshesNew[index].Material, sizeof(renderer::Material));
            if (!mbActiveEmissive)
            {
                cbMaterial.Emissive = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }
            Renderer::GetInstance()->UpdateCB(Renderer::eCbType::CbMaterial, &cbMaterial);

            deviceContext->DrawIndexed(static_cast<uint32_t>(mMeshesNew[index].IndexRange.Count), mMeshesNew[index].IndexRange.StartIndex, mMeshesNew[index].VertexRange.StartIndex);
        }

        Renderer::GetInstance()->UnbindTexturePs(2);
        SAFETY_RELEASE(deviceContext);
    }

    void Model::DrawShadowNew()
    {
        Renderer::GetInstance()->SetInputLayoutTo(Renderer::eInputLayout::P);

        BufferManager* const bufferManager = Renderer::GetInstance()->GetBufferManager();
        // TODO: cleanup - 미사용 코드 제거
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        const uint32 stride = sizeof(renderer::Vertex);

        Renderer::GetInstance()->BindVertexBuffer(stride, 0);
        Renderer::GetInstance()->BindIndexBuffer(0);

        Renderer::GetInstance()->SetRasterState(Renderer::eRasterType::Outline);
        Renderer::GetInstance()->SetShaderTo(Renderer::eShader::Shadow);

        Renderer::GetInstance()->BindCbToVsByType(0U, 1U, Renderer::eCbType::CbWorld);
        Renderer::GetInstance()->BindCbToVsByType(1U, 1U, Renderer::eCbType::CbLightViewProjMatrix);

        // Draw
        ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();
        for (size_t index = 0U; index < mMeshesNew.size(); ++index)
        {
            deviceContext->DrawIndexed(static_cast<uint32_t>(mMeshesNew[index].IndexRange.Count), mMeshesNew[index].IndexRange.StartIndex, mMeshesNew[index].VertexRange.StartIndex);
        }
        SAFETY_RELEASE(deviceContext);
    }

    void Model::Update()
    {
        Renderer::CbWorld cbWorld;
        cbWorld.Matrix = XMMatrixTranspose(mMatWorld);

        Renderer::GetInstance()->UpdateCB(Renderer::eCbType::CbWorld, &cbWorld);

    }


    void Model::SetMeshes(std::vector<MeshNew>& meshes)
    {
        ASSERT(mMeshesNew.empty(), "mMeshes is not empty.");
        mMeshesNew.swap(meshes);
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
