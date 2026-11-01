#include "Model.h"
#include "../Renderer.h"
#include "../../Util/Macro.h"

namespace renderer
{
    Model::Model()
        : mMesh()
        , mMatRotation(XMMatrixIdentity())
        , mMatScale(XMMatrixIdentity())
        , mbHighlight(false)
        , mbActiveEmissive(false)
    {

        mMatWorld  = XMMatrixIdentity();
    }

    Model::~Model()
    {
    }

    void Model::SubmitCommand()
    {
    }

    void Model::Draw(Renderer& renderer)
    {
        PushConstantBlock pushConstantBlock{ mMatWorld };
        renderer.UploadPushConstant(pushConstantBlock, eShader::Simple);
        for (const auto& subMesh : mMesh.SubMeshes)
        {
            renderer.DrawIndexed(subMesh.IndexRange.Count, subMesh.VertexRange.StartIndex, subMesh.IndexRange.StartIndex);
        }
    }

    void Model::Update()
    {
        // MEMO: 전치 안 해도 되지만, 나중에 필요해질 테니 까먹지 않도록 미리 구성해둠.
        mMatWorld = XMMatrixTranspose(XMMatrixIdentity());
    }

    void Model::SetMesh(const Mesh& mesh)
    {
        mMesh = std::move(mesh);
    }

    void Model::SetHighlight(bool bSelection)
    {
        mbHighlight = bSelection;
    }

    int32_t Model::GetSubMeshCount() const
    {
        return mMesh.SubMeshes.size();
    }

    XMFLOAT3 Model::GetCenterPoint(int32_t subMeshIndex) const
    {
        ASSERT((subMeshIndex >= 0 && subMeshIndex < mMesh.SubMeshes.size()), "유효하지 않은 SubMeshIndex. pass(%d), validSubMeshCount(%d)", subMeshIndex, mMesh.SubMeshes.size());

        const XMVECTOR minBound = XMLoadFloat3(&mMesh.SubMeshes[subMeshIndex].MinBound);
        const XMVECTOR maxBound = XMLoadFloat3(&mMesh.SubMeshes[subMeshIndex].MaxBound);
        const XMVECTOR mid = (minBound + maxBound) * 0.5f;
        XMFLOAT3 centerPosition;
        XMStoreFloat3(&centerPosition, mid);
        return centerPosition;
    }

    XMMATRIX Model::GetMatrix() const
    {
        return mMatWorld;
    }
}
