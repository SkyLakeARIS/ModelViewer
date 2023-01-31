#pragma once
#include <map>
#include <set>

#include "framework.h"
#include "Model.h"

class ModelImporter final
{

    struct ObjectNode
    {
        FbxNode* parent;
        FbxNode* current;
    };

public:
            ModelImporter();
    virtual ~ModelImporter();

    void                            Initialize();
    void                            Release();

    void                            LoadFbxModel(const char* fileName);

    const std::vector<Mesh>*        GetMesh() const;
    size_t                          GetMeshCount() const;

private:

    void    preprocess(FbxNode* parent, FbxNode* current);

    void    parseMesh();

private:

    FbxManager*     mFbxManager;
    FbxImporter*    mImporter;
    FbxScene*       mFbxScene;
    FbxIOSettings*  mSetting;

    std::vector<ObjectNode> mFbxObjects;
    std::vector<Mesh>       mMeshes;
    std::set<int>           mVertexDuplicationCheck;    // 폴리곤을 이용하여 모델을 구성하면 버텍스 중복이 생기므로 제거
    std::map<int, int>      mIndexMap;                  // 올바른 인덱스리스트 구성을 위한 PolygonVertex와 벡터와 연결
};
