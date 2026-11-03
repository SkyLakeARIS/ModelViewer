#pragma once
#include <vector>
#include <unordered_map>
#include "Material.h"
#include "RenderTypes.h"
#include "../../Util/Define.h"

namespace renderer
{

    // MEMO: SubMesh가 Mesh의 부분집합
    // MEMO: SubMesh가 Material을 가지고, Mesh가 SubMesh들을 가지는 구조로 정리.
    // 캐릭터 모델이 SubMesh별로 다른 Material을 가지므로 이 구조가 대응이 가능한 구조
    struct SubMesh
    {
        SemiMaterial Material;
        // range in Mesh, in ElementCount.
        BufferRange VertexRange;
        BufferRange IndexRange;
        HashID SubMeshHash;
        // For Orbit Camera
        XMFLOAT3 MinBound;
        XMFLOAT3 MaxBound;
        // for debugging
        int8_t SubMeshName[util::MAX_NAME_LENGTH];
        // MEMO: RenderPass별 PipelineHash
        std::unordered_map<eRenderPass, HashID> Pipelines;
    };

    struct Mesh
    {
        eVertexFormat VertexFormat;
        HashID MeshHash;
        // Range in Buffer, in ElementCount.
        BufferRange VertexRange;
        BufferRange IndexRange;
        std::vector<SubMesh> SubMeshes;
        // for debugging
        int8_t MeshName[util::MAX_NAME_LENGTH];
    };
}
