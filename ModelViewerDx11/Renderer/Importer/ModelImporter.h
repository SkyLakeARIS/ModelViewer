#pragma once
#include "../../framework.h"
#include "../Resources/ModelData.h"

namespace renderer
{
    struct ImportedModelContainer;
    class Renderer;

    class ModelImporter
    {
    public:

        ModelImporter();
        ~ModelImporter();

        void Initialize();
        void Release();

        // MEMO: 우선은 동기식이니까 공간을 넘겨주고 Importer가 데이터를 채워주도록 하고, 나중에 고도화 하자.
        void LoadFbxModel(const int8_t* const fileName, HashID& outModelHash, ImportedModelContainer& outModelContainer);

    private:

        void preprocess(FbxNode* parent, FbxNode* current, std::vector<FbxNode*>& outNodes);

        void parseMesh(std::vector<FbxNode*>& outNodes, ImportedModelContainer& outModelContainer, FbxVector4& outMinBound, FbxVector4& outMaxBound);

        void parseTextureInfo(std::vector<FbxNode*>& outNodes, ImportedModelContainer& outModelContainer);

        //  sdk 문서에서 가져온 정보 출력용 함수
        void parseMaterial(std::vector<FbxNode*>& outNodes, ImportedModelContainer& outModelContainer);

        static const FbxImplementation* LookForImplementation(FbxSurfaceMaterial* pMaterial);


        // temp. will be destroy / sdk 문서에서 가져온 정보 출력용 함수
        void DisplayString(const char* pHeader, const char* pValue = "", const char* pSuffix = "");
        void DisplayBool(const char* pHeader, bool pValue, const char* pSuffix = "");
        void DisplayInt(const char* pHeader, int pValue, const char* pSuffix = "");
        void DisplayDouble(const char* pHeader, double pValue, const char* pSuffix = "");
        void Display2DVector(const char* pHeader, FbxVector2 pValue, const char* pSuffix = "");
        void Display3DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix = "");
        void DisplayColor(const char* pHeader, FbxColor pValue, const char* pSuffix = "");
        void Display4DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix = "");
        void PrintString(FbxString& pString);

    private:

        static const int8_t* const TEXTURE_FILE_PATH_A;
        static const int8_t* const TEXTURE_FILE_EXTENSION_A;
        static const int8_t NORMAL_TEXTURE_FILE_SUFFIX_A;

        FbxManager* mFbxManager;
        FbxImporter* mImporter;
        FbxScene* mFbxScene;
        FbxIOSettings* mSetting;
    };
}
