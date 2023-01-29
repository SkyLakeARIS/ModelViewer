#include "pch.h"
#include "Model.h"

Model::Model()
{
    
}

Model::~Model()
{
    
}

void Model::Render(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView** resourceView, const size_t numTexture)
{
    /*
     *  render에 이건 좀 아닌듯..
     *  그냥 GetMeshListReference를 없애고, 외부에서 벡터를 받아서 초기화하고
     *  나머지 정보도 초기화 하는걸로.
     *  (렌더 준비에 오래걸리겠지만, 렌더링이 느려지는 것 보단 나을 것)
     */
    const size_t numMesh = mMeshes.size();
    size_t offset = 0;
    for (size_t index = 0; index < numMesh; ++index)
    {

        deviceContext->PSSetShaderResources(0, 1, &resourceView[index]);
        /*
         *  문제점
         *  외부에서 받은 vertex buffer에 써넣어주는 방식에
         *  이 방식으로 메시를 그리면, buffer에서 어느 부분이 이 모델의 정보인지 알 수 없으므로
         *  의도하지 않은 렌더링 결과를 내놓을 수 있을것.
         *  -현재는 모델 하나만 그리기 때문에 아직까지 상관은 없음.
         *  - 나중에 외부에서 버퍼를 관리하고 모델은 버퍼에서 자신의 정보에 대한 위치만 가지고 있는것도
         */
        deviceContext->Draw(mMeshes[index].Vertex.size(), offset);

        offset += mMeshes[index].Vertex.size();
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

std::vector<Mesh> Model::GetMeshList() const
{
    return mMeshes;
}

std::vector<Mesh>* Model::GetMeshListReference()
{
    return &mMeshes;
}

