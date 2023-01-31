#include "pch.h"
#include "Model.h"

#include "ModelImporter.h"

Model::Model()
    : mNumMesh(0)
    , mNumVertex(0)
{
    
}

Model::~Model()
{
    
}

void Model::Render(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView** resourceView, const size_t numTexture)
{
    size_t vertexOffset = 0;
    size_t indexOffset = 0;
    for (size_t index = 0; index < mNumMesh; ++index)
    {
        /*
         *  이 방법외에 다른 방법으로 적용이 필요하다.
         */
        if (index < 4)
        {
            deviceContext->PSSetShaderResources(0, 1, &resourceView[index]);
        }
        else
        {
            deviceContext->PSSetShaderResources(0, 1, &resourceView[3]);
        }
        /*
         *  문제점
         *  외부에서 받은 vertex buffer에 써넣어주는 방식에
         *  이 방식으로 메시를 그리면, buffer에서 어느 부분이 이 모델의 정보인지 알 수 없으므로
         *  의도하지 않은 렌더링 결과를 내놓을 수 있을것.
         *  -현재는 모델 하나만 그리기 때문에 아직까지 상관은 없음.
         *  - 나중에 외부에서 버퍼를 관리하고 모델은 버퍼에서 자신의 정보에 대한 위치만 가지고 있는것도
         */
         //deviceContext->Draw(mMeshes[index].Vertex.size(), offset);
        deviceContext->DrawIndexed(mMeshes[index].IndexList.size(), indexOffset, vertexOffset);

        vertexOffset += mMeshes[index].Vertex.size();
        indexOffset += mMeshes[index].IndexList.size();
    }
}

void Model::SetupMesh(ModelImporter& importer)
{
    const std::vector<Mesh>& meshes = *importer.GetMesh();

    mNumMesh = meshes.size();
    mMeshes.reserve(mNumMesh);
    for (size_t meshIndex = 0; meshIndex < mNumMesh; ++meshIndex)
    {
        mMeshes.push_back(Mesh());
        Mesh& mesh = mMeshes.back();

        mNumVertex = meshes[meshIndex].Vertex.size();
        mMeshes[meshIndex].Vertex.reserve(mNumVertex);
        for (size_t vertexIndex = 0; vertexIndex < mNumVertex; ++vertexIndex)
        {
            mesh.Vertex.push_back(VertexInfo());
            VertexInfo& vertex = mesh.Vertex.back();

            vertex.Pos = meshes[meshIndex].Vertex[vertexIndex].Pos;
            vertex.Norm = meshes[meshIndex].Vertex[vertexIndex].Norm;
            vertex.Tex = meshes[meshIndex].Vertex[vertexIndex].Tex;
        }

        const int numIndexList = meshes[meshIndex].IndexList.size();
        mMeshes[meshIndex].IndexList.reserve(numIndexList);
        for (size_t index = 0; index < numIndexList; ++index)
        {
            mesh.IndexList.push_back(meshes[meshIndex].IndexList[index]);
        }

        mesh.HasTexture = meshes[meshIndex].HasTexture;
        wcscpy_s(mesh.Name, MESH_NAME_LENGTH, meshes[meshIndex].Name);
        mesh.Name[MESH_NAME_LENGTH - 1] = (WCHAR)L"\n";
    }
}

void Model::UpdateVertexBuffer(VertexInfo* buffer, size_t bufferSize, size_t startIndex)
{
    ASSERT(buffer != nullptr, "버퍼는 nullptr가 아니어야 합니다.");

    const size_t numMesh = mMeshes.size();
    size_t offset = 0;
    for (size_t meshIndex = 0; meshIndex < numMesh; ++meshIndex)
    {
        const size_t numVertex = mMeshes[meshIndex].Vertex.size();
        for (size_t vertexIndex = 0; vertexIndex < numVertex; ++vertexIndex)
        {
            const size_t bufferIndex = startIndex + offset + vertexIndex;
            buffer[bufferIndex].Pos = mMeshes[meshIndex].Vertex[vertexIndex].Pos;
            buffer[bufferIndex].Norm = mMeshes[meshIndex].Vertex[vertexIndex].Norm;
            buffer[bufferIndex].Tex = mMeshes[meshIndex].Vertex[vertexIndex].Tex;
        }
        offset += numVertex;
    }
    
}

void Model::UpdateIndexBuffer(size_t* buffer, size_t bufferSize, size_t startIndex)
{
    ASSERT(buffer != nullptr, "버퍼는 nullptr가 아니어야 합니다.");

    const size_t numMesh = mMeshes.size();
    size_t offset = 0;
    for (size_t meshIndex = 0; meshIndex < numMesh; ++meshIndex)
    {
        const size_t numIndex = mMeshes[meshIndex].IndexList.size();
        for (size_t index = 0; index < numIndex; ++index)
        {
            const size_t bufferIndex = startIndex + offset + index;
            buffer[bufferIndex] = mMeshes[meshIndex].IndexList[index];
        }
        offset += numIndex;
    }

}

size_t Model::GetMeshCount() const
{
    return mMeshes.size();

}

const WCHAR* Model::GetMeshName(size_t meshIndex) const
{
    ASSERT(meshIndex < mMeshes.size(), "인덱스 범위를 벗어났습니다.");

    return mMeshes[meshIndex].Name;
}

size_t Model::GetVertexCount(size_t meshIndex) const
{
    ASSERT(meshIndex < mMeshes[meshIndex].Vertex.size(), "인덱스 범위를 벗어났습니다.");
    return mMeshes[meshIndex].Vertex.size();
}

size_t Model::GetIndexListCount(size_t meshIndex) const
{
    ASSERT(meshIndex < mMeshes[meshIndex].Vertex.size(), "인덱스 범위를 벗어났습니다.");
    return mMeshes[meshIndex].IndexList.size();
}

