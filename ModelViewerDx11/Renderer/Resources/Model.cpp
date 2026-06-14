#include "Model.h"
#include "BufferManager.h"
#include "../Importer/ModelImporter.h"
#include "../../Util/Util.h"
#include "../../Util/Macro.h"

namespace renderer
{
    Model::Model(scene::Camera* camera, const int8_t* const filePath)
        : mCamera(camera)
        , mModelHash(0)
        , mNumMesh(0)
        , mNumVertex(0)
        , mVertices(nullptr)
        , mIndices(nullptr)
        , mCenterPosition(0.0f, 0.0f, 0.0f)
        , mMatRotation(XMMatrixIdentity())
        , mMatScale(XMMatrixIdentity())
        , mLight(nullptr)
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

        mMeshes.clear();







        delete[] mVertices;
        delete[] mIndices;

    }

    void Model::Draw()
    {
        Renderer::GetInstance()->SetInputLayoutTo(Renderer::eInputLayout::PTN);

        BufferManager* const bufferManager = Renderer::GetInstance()->GetBufferManager();
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        const uint32 stride = sizeof(renderer::Vertex);
        const uint32 offset = vertexRange.StartIndex;

        Renderer::GetInstance()->BindVertexBuffer(stride, offset);
        Renderer::GetInstance()->BindIndexBuffer(indexRange.StartIndex);

        // outline
        int32_t vertexOffset = 0;
        uint32_t indexOffset = 0U;

        ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();

        if (mbHighlight)
        {

            Renderer::GetInstance()->SetRasterState(Renderer::eRasterType::CullBack);

            Renderer::GetInstance()->SetShaderTo(Renderer::eShader::Outline);
            Renderer::GetInstance()->BindCbToVsByType(0U, 1U, Renderer::eCbType::CbWorld);
            Renderer::GetInstance()->BindCbToVsByType(1U, 1U, Renderer::eCbType::CbOutlineProperty);
            Renderer::GetInstance()->BindCbToVsByType(2U, 1U, Renderer::eCbType::CbViewProj);


            for (uint32_t index = 0U; index < mNumMesh; ++index)
            {
                deviceContext->DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexList.size()), indexOffset, vertexOffset);

                vertexOffset += static_cast<int32_t>(mMeshes[index].Vertex.size());
                indexOffset += static_cast<uint32_t>(mMeshes[index].IndexList.size());
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

        auto* texShadow = Renderer::GetInstance()->GetShadowTexture();
        deviceContext->PSSetShaderResources(2U, 1U, &texShadow);
        texShadow->Release();

        // Draw
        // 위에 따라서 변경 필요. 아니면 원래 방법대로 VertexBuffer를 하나로 뭉쳐야 함.
        vertexOffset = 0;
        indexOffset = 0U;
        for (size_t index = 0U; index < mNumMesh; ++index)
        {
            deviceContext->PSSetShaderResources(0U, 1U, &mMeshes[index].Texture);
            deviceContext->PSSetShaderResources(1U, 1U, &mMeshes[index].TextureNormal);
            CbMaterial cbMaterial;
            ZeroMemory(&cbMaterial, sizeof(CbMaterial));

            memcpy(&cbMaterial, &mMeshes[index].Material, sizeof(renderer::Material));
            if (!mbActiveEmissive)
            {
                cbMaterial.Emissive = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }
            Renderer::GetInstance()->UpdateCB(Renderer::eCbType::CbMaterial, &cbMaterial);

            deviceContext->DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexList.size()), indexOffset, vertexOffset);

            vertexOffset += static_cast<int32_t>(mMeshes[index].Vertex.size());
            indexOffset += static_cast<uint32_t>(mMeshes[index].IndexList.size());

        }

        ID3D11ShaderResourceView* unbind = nullptr;
        // TODO: textureManager 만들면 Bind 함수만들어서 이동
        deviceContext->PSSetShaderResources(2U, 1U, &unbind);
        SAFETY_RELEASE(deviceContext);
    }

    void Model::DrawShadow()
    {
        Renderer::GetInstance()->SetInputLayoutTo(Renderer::eInputLayout::P);

        BufferManager* const bufferManager = Renderer::GetInstance()->GetBufferManager();
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        const uint32 stride = sizeof(renderer::Vertex);
        const uint32 offset = vertexRange.StartIndex;

        Renderer::GetInstance()->BindVertexBuffer(stride, offset);
        Renderer::GetInstance()->BindIndexBuffer(indexRange.StartIndex);

        Renderer::GetInstance()->SetRasterState(Renderer::eRasterType::Outline);
        Renderer::GetInstance()->SetShaderTo(Renderer::eShader::Shadow);

        Renderer::GetInstance()->BindCbToVsByType(0U, 1U, Renderer::eCbType::CbWorld);
        Renderer::GetInstance()->BindCbToVsByType(1U, 1U, Renderer::eCbType::CbLightViewProjMatrix);


        // Draw
        int32_t vertexOffset = 0;
        uint32_t indexOffset = 0U;
        ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();
        for (size_t index = 0U; index < mNumMesh; ++index)
        {
            deviceContext->DrawIndexed(static_cast<uint32_t>(mMeshes[index].IndexList.size()), indexOffset, vertexOffset);

            vertexOffset += static_cast<int32_t>(mMeshes[index].Vertex.size());
            indexOffset += static_cast<uint32_t>(mMeshes[index].IndexList.size());
        }
        SAFETY_RELEASE(deviceContext);
    }

    void Model::DrawNew()
    {
        Renderer::GetInstance()->SetInputLayoutTo(Renderer::eInputLayout::PTN);

        BufferManager* const bufferManager = Renderer::GetInstance()->GetBufferManager();
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        // TODO: cleanup - 더이상 필요 없어진 자잘한 코드들 정리 필요
        const uint32 stride = sizeof(renderer::Vertex);
        const uint32 offset = vertexRange.StartIndex;

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
        const BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);

        // TODO: cleanup - 더이상 필요 없어진 자잘한 코드들 정리 필요
        const uint32 stride = sizeof(renderer::Vertex);
        const uint32 offset = vertexRange.StartIndex;

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

    void Model::SetLight(scene::Light* light)
    {
        mLight = light;
    }

    HRESULT Model::SetupMesh(ModelImporter& importer)
    {
        // vertex / index list 데이터 합치기 용
        const uint32 sumVertexCount = importer.GetSumVertexCount();
        const uint32 sumIndexCount = importer.GetSumIndexCount();

        // 하나로 뭉침
        Vertex* vertices = new renderer::Vertex[sumVertexCount];
        uint32_t* indices = new uint32[sumIndexCount];

        // mesh - vertices, indices, textures ...
        mNumMesh = importer.GetMeshCount();
        ASSERT(mNumMesh > 0U, "Model::SetupMesh 메시 수는 1 이상이어야 합니다. num of mesh must be over 0.");

        renderer::Vertex* posInVertices = vertices;
        uint32* posInIndices = indices;
        uint32 offset = 0U;
        for (size_t meshIndex = 0U; meshIndex < mNumMesh; ++meshIndex)
        {
            renderer::Mesh* mesh = importer.GetMesh(meshIndex);
            renderer::Mesh newMesh;
            newMesh.Vertex.swap(mesh->Vertex);
            newMesh.IndexList.swap(mesh->IndexList);
            newMesh.bLightMap = mesh->bLightMap;
            memcpy(newMesh.Name, mesh->Name, sizeof(WCHAR) * renderer::MESH_NAME_LENGTH);
            newMesh.Texture = mesh->Texture;
            mesh->Texture = nullptr;
            newMesh.TextureNormal = mesh->TextureNormal;
            mesh->TextureNormal = nullptr;
            newMesh.NumTexuture = mesh->NumTexuture;
            memcpy(&newMesh.Material, &mesh->Material, sizeof(renderer::Material));

            mMeshes.push_back(newMesh);

            memcpy_s(posInVertices, sizeof(renderer::Vertex) * sumVertexCount, mMeshes[meshIndex].Vertex.data(), sizeof(renderer::Vertex) * mMeshes[meshIndex].Vertex.size());
            posInVertices += mMeshes[meshIndex].Vertex.size();

            memcpy_s(posInIndices, sizeof(uint32) * sumIndexCount, mMeshes[meshIndex].IndexList.data(), sizeof(uint32) * mMeshes[meshIndex].IndexList.size());
            posInIndices += mMeshes[meshIndex].IndexList.size();

            // model importer로 이동하는게 좋을 것 같다.
            XMFLOAT3 minHeight = XMFLOAT3(100000.0f, 100000.0f, 100000.0f);
            XMFLOAT3 maxHeight = XMFLOAT3(0.0f, 0.0f, 0.0f);
            for (size_t vertexIndex = 0; vertexIndex < mMeshes[meshIndex].Vertex.size(); ++vertexIndex)
            {

                if (maxHeight.y < mMeshes[meshIndex].Vertex[vertexIndex].Position.y)
                {
                    maxHeight = mMeshes[meshIndex].Vertex[vertexIndex].Position;
                }
                if (minHeight.y > mMeshes[meshIndex].Vertex[vertexIndex].Position.y)
                {
                    minHeight = mMeshes[meshIndex].Vertex[vertexIndex].Position;
                }
            }


            // calc center of object

            // 센터 값이 바닥쪽에 있음.
            // 어떻게 보면 아래 방식이 뭔가 모델링 툴의 진짜 원점같아 보이긴 한다. 그래서 일단 남겨놓을 예정.
            XMVECTOR min = XMLoadFloat3(&minHeight);
            XMVECTOR max = XMLoadFloat3(&maxHeight);
            max += min;
            XMStoreFloat3(&mCenterPosition, (max / 2.0f));

            // 물체의 정중앙을 초점으로 삼고 싶기 때문에 변경
            //mCenterPosition = importer.GetModelCenter();
        }

        // and make d3d buffer

        BufferManager* bufferManager = Renderer::GetInstance()->GetBufferManager();

        bufferManager->AddVertexData(reinterpret_cast<int8_t*>(vertices), sizeof(Vertex) * sumVertexCount, mModelHash);
        bufferManager->AddIndexData(reinterpret_cast<int8_t*>(indices), sizeof(uint32_t) * sumIndexCount, mModelHash);

        delete[] vertices;
        delete[] indices;

        D3D11_BUFFER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));

        HRESULT result = S_OK;



        return S_OK;
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

    //
    //void Model::UpdateVertexBuffer(Vertex* buffer, size_t bufferSize, size_t startIndex)
    //{
    //    ASSERT(buffer != nullptr, "버퍼는 nullptr가 아니어야 합니다.");
    //
    //    const size_t numMesh = mMeshes.size();
    //    size_t offset = 0;
    //    for (size_t meshIndex = 0; meshIndex < numMesh; ++meshIndex)
    //    {
    //        const size_t numVertex = mMeshes[meshIndex].Vertex.size();
    //        for (size_t vertexIndex = 0; vertexIndex < numVertex; ++vertexIndex)
    //        {
    //            const size_t bufferIndex = startIndex + offset + vertexIndex;
    //            buffer[bufferIndex].Position = mMeshes[meshIndex].Vertex[vertexIndex].Position;
    //            buffer[bufferIndex].Normal = mMeshes[meshIndex].Vertex[vertexIndex].Normal;
    //            buffer[bufferIndex].TexCoord = mMeshes[meshIndex].Vertex[vertexIndex].TexCoord;
    //        }
    //        offset += numVertex;
    //    }
    //    
    //}
    //
    //void Model::UpdateIndexBuffer(unsigned int* buffer, size_t bufferSize, size_t startIndex)
    //{
    //    ASSERT(buffer != nullptr, "버퍼는 nullptr가 아니어야 합니다.");
    //
    //    const size_t numMesh = mMeshes.size();
    //    size_t offset = 0;
    //    for (size_t meshIndex = 0; meshIndex < numMesh; ++meshIndex)
    //    {
    //        const size_t numIndex = mMeshes[meshIndex].IndexList.size();
    //        for (size_t index = 0; index < numIndex; ++index)
    //        {
    //            const size_t bufferIndex = startIndex + offset + index;
    //            buffer[bufferIndex] = mMeshes[meshIndex].IndexList[index];
    //        }
    //        offset += numIndex;
    //    }
    //
    //}

    size_t Model::GetMeshCount() const
    {
        return mMeshes.size();

    }

    // TODO: cleanup - 안쓰는 함수는 제거
    //size_t Model::GetVertexCount(size_t meshIndex) const
    //{
    //    ASSERT(meshIndex < mMeshes.size(), "인덱스 범위를 벗어났습니다.");
    //    return mMeshes[meshIndex].VertexInfo.Count;
    //}

    //size_t Model::GetIndexListCount(size_t meshIndex) const
    //{
    //    ASSERT(meshIndex < mMeshes.size(), "인덱스 범위를 벗어났습니다.");
    //    return mMeshes[meshIndex].IndexInfo.Count;
    //}

    XMFLOAT3 Model::GetCenterPoint() const
    {
        XMFLOAT3 pos = mCenterPosition;
        pos.y += 1.0f;
        return pos;
    }

    void Model::prepare()
    {
        // 아래 두 인자에 대해서는 아직 제대로 이해하지 못함. 여러개를 전달하는건 삽질과 검색이 필요할 듯.
        //const uint32 stride = sizeof(Vertex); // 대개 Vertex가 서로 다른 stride()가질 때 사용하는 듯, 예로 Pos+nor+tex 인 vertex 하나 pos+tex인 vertex하나
        //const uint32 offset = 0;
        //mDeviceContext->IASetVertexBuffers(0, mNumMesh, mVertexBuffers, &stride, &offset);
        //mDeviceContext->IASetIndexBuffer(gIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    }
}
