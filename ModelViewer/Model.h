#pragma once
#include "framework.h"

class ModelImporter;

constexpr size_t MESH_NAME_LENGTH = 128;

struct VertexInfo
{
    XMFLOAT3 Pos;
    XMFLOAT3 Norm;
    XMFLOAT2 Tex;
};

struct Mesh
{
    WCHAR Name[MESH_NAME_LENGTH];
    std::vector<VertexInfo> Vertex;
    std::vector<int> IndexList;
    bool HasTexture;
};

class Model final
{

public:
    Model();
    virtual ~Model();

    void                Render(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView** resourceView, const size_t numTexture);

    void                SetupMesh(ModelImporter& importer);

    void                UpdateVertexBuffer(VertexInfo* buffer, size_t bufferSize, size_t startIndex);
    void                UpdateIndexBuffer(size_t* buffer, size_t bufferSize, size_t startIndex);

    size_t              GetMeshCount() const;
    const WCHAR*        GetMeshName(size_t meshIndex) const;

    size_t              GetVertexCount(size_t meshIndex) const;
    size_t              GetIndexListCount(size_t meshIndex) const;

private:
    size_t mNumMesh;
    size_t mNumVertex;

    std::vector<Mesh> mMeshes;
};


/*
 *  anbi TM
 */
 ////  최상위
 //mMatRootTM = XMMATRIX(
 //    39.370080, 0.000000, 0.000000, 0.000000,
 //    0.000000, -0.000002, -39.370080, 0.000000,
 //    0.000000, 39.370080, -0.000002, 0.000000,
 //    0.000000, 0.000000, 0.000000, 1.000000);

 //// 그다음
 //mMatSubTM = XMMATRIX(
 //    1.000000, 0.000000, 0.000000, 0.000000,
 //    0.000000, 1.000000, 0.000000, 0.000000,
 //    0.000000, -0.000000, 1.000000, 0.000000,
 //    0.000000, 0.000000, 0.000000, 1.000000);

 //mMatBodyTM = XMMATRIX(
 //    1.000000, 0.000000, 0.000000, 0.000000,
 //    0.000000, -0.000000, 1.000000, 0.000000,
 //    0.000000, -1.000000, -0.000000, 0.000000,
 //    0.000000, 0.000000, 0.000000, 1.000000);

 //mMatFaceTM = XMMATRIX(
 //    1.000000, 0.000000, 0.000000, 0.000000,
 //    0.000000, -0.000000, 1.000000, 0.000000,
 //    0.000000, -1.000000, -0.000000, 0.000000,
 //    0.000000, 0.000000, 0.000000, 1.000000);

 //mMatHairTM = XMMATRIX(
 //    1.000000, 0.000000, 0.000000, 0.000000,
 //    0.000000, -0.000000, 1.000000, 0.000000,
 //    0.000000, -1.000000, -0.000000, 0.000000,
 //    0.000000, 0.000000, 0.000000, 1.000000);