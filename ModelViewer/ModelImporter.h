#pragma once
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

    void            Initialize();
    void            Release();

    void            LoadFBXAndCreateModel(const char* fileName, Model** modelToLoad);

private:

    void            printNode(FbxNode* node);
    FbxString       getAttributeTypeName(FbxNodeAttribute::EType type);
    void            printAttribute(FbxNodeAttribute* pAttribute);

    void            preprocess(FbxNode* parent, FbxNode* current);

    void            parseMesh(std::vector<Mesh>& modelToCreate);

private:

    FbxManager*     mFbxManager;
    FbxImporter*    mImporter;
    FbxScene*       mFbxScene;
    FbxIOSettings*  mSetting;

    std::vector<ObjectNode>     mFbxObjects;
};
