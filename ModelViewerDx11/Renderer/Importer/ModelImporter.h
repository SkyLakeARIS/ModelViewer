#pragma once
#include "../../framework.h"
#include <map>
#include <set>
#include "../Resources/ModelData.h"

namespace renderer
{
    class ModelImporter
    {

        struct MeshForImport
        {
            FbxNode* Parent;
            FbxNode* Current;
            renderer::Mesh     Mesh;
        };

    public:

        ModelImporter(ID3D11Device* device);
        ~ModelImporter();

        void                            Initialize();
        void                            Release();

        void                            LoadFbxModel(const char* fileName);

        renderer::Mesh* GetMesh(size_t meshIndex);
        size_t                          GetMeshCount() const;
        uint32                          GetSumVertexCount() const;
        uint32                          GetSumIndexCount() const;

        XMFLOAT3                        GetModelCenter() const;
    private:

        void    preprocess(FbxNode* parent, FbxNode* current);

        void    parseMesh();

        void    parseTextureInfo();

        //  sdk 문서에서 가져온 정보 출력용 함수
        void    parseMaterial();

        static const FbxImplementation* LookForImplementation(FbxSurfaceMaterial* pMaterial);

        HRESULT loadTextureFromFileAndCreateResource(
            const WCHAR* fileName,
            const D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc,
            ID3D11ShaderResourceView** outShaderResourceView);

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

        static const wchar_t* TEXTURE_FILE_PATH;
        static const wchar_t* NORMAL_TEXTURE_FILE_SUFFIX;
        static const wchar_t* TEXTURE_FILE_EXTENSION;

        ID3D11Device* mDevice;


        FbxManager* mFbxManager;
        FbxImporter* mImporter;
        FbxScene* mFbxScene;
        FbxIOSettings* mSetting;

        std::vector<MeshForImport>  mFbxObjects;
        //std::vector<Mesh>           mMeshes;
        uint32                      mSumVertexCount;    // 메시 버텍스를 하나로 뭉치기 위함.
        uint32                      mSumIndexCount;     // 메시 버텍스를 하나로 뭉치기 위함.

        FbxVector4                  mModelCenter;

        std::set<int>               mVertexDuplicationCheck;    // 폴리곤을 이용하여 모델을 구성하면 버텍스 중복이 생기므로 제거
        std::map<int, int>          mIndexMap;                  // 올바른 인덱스리스트 구성을 위한 PolygonVertex와 벡터와 연결
    };
}