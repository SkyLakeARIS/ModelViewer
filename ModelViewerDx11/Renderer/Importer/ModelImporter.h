#pragma once
#include "../../framework.h"
#include <map>
#include <set>
#include "../Resources/ModelData.h"

namespace renderer
{
    struct ImportedModelContainer;
    class Renderer;

    // TODO: 우선 모델을 하나만 로드하니까 원활한 리팩토링을 위해 최소한으로만 개선하고, 나중에 여러 개를 로드할 수 있도록 확장이 필요함
    class ModelImporter
    {

        struct MeshForImport
        {
            FbxNode* Parent;
            FbxNode* Current;
            renderer::Mesh     Mesh;
        };

    public:

        ModelImporter();
        ~ModelImporter();

        void                            Initialize();
        void                            Release();

        void LoadFbxModel(const char* fileName, Renderer* const renderer);
        // MEMO: 우선은 동기식이니까 공간을 넘겨주고 Importer가 데이터를 채워주도록 하고, 나중에 고도화 하자.
        void LoadFbxModelNew(const int8_t* const fileName, HashID& outModelHash, ImportedModelContainer& outModelContainer);

        renderer::Mesh* GetMesh(size_t meshIndex);
        size_t                          GetMeshCount() const;
        uint32                          GetSumVertexCount() const;
        uint32                          GetSumIndexCount() const;

        XMFLOAT3                        GetModelCenter() const;
    private:

        void preprocess(FbxNode* parent, FbxNode* current);
        void preprocessNew(FbxNode* parent, FbxNode* current, std::vector<FbxNode*>& outNodes);

        void    parseMesh();
        void    parseMeshNew(std::vector<FbxNode*>& outNodes, ImportedModelContainer& outModelContainer, FbxVector4& outMinBound, FbxVector4&
                             outMaxBound);

        // TODO: 파일 경로 문자열을(해시를) 넘겨서 TextureManager가 로드해서 관리하도록 (데이터 로드는 또 다른 매니저가 처리하는 게 좋을지?)
        void    parseTextureInfo(Renderer* const renderer);
        void    parseTextureInfoNew(std::vector<FbxNode*>& outNodes, ImportedModelContainer& outModelContainer);

        //  sdk 문서에서 가져온 정보 출력용 함수
        void    parseMaterial();
        void    parseMaterialNew(std::vector<FbxNode*>& outNodes, ImportedModelContainer& outModelContainer);

        bool                            loadTextureImageW(const wchar_t* const filePath, ScratchImage& outTexImage);
        static const FbxImplementation* LookForImplementation(FbxSurfaceMaterial* pMaterial);

        HRESULT loadTextureFromFileAndCreateResource(
            const WCHAR*                           fileName,
            Renderer* const                        renderer,
            const D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc, ID3D11ShaderResourceView** outShaderResourceView);

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

        static const wchar_t* TEXTURE_FILE_PATH_W;
        static const wchar_t* NORMAL_TEXTURE_FILE_SUFFIX_W;
        static const wchar_t* TEXTURE_FILE_EXTENSION_W;

        static const int8_t* const NORMAL_TEXTURE_FILE_SUFFIX_A;
        static const int8_t* const TEXTURE_FILE_PATH_A;
        static const int8_t* const TEXTURE_FILE_EXTENSION_A;

        FbxManager* mFbxManager;
        FbxImporter* mImporter;
        FbxScene* mFbxScene;
        FbxIOSettings* mSetting;


        // TODO: 개선 이후 제거 대상
        std::vector<MeshForImport>  mFbxObjects;
        //std::vector<Mesh>           mMeshes;
        // TODO: 개선 이후 제거 대상
        uint32                      mSumVertexCount;    // 메시 버텍스를 하나로 뭉치기 위함.
        uint32                      mSumIndexCount;     // 메시 버텍스를 하나로 뭉치기 위함.

        FbxVector4                  mModelCenter;

        // TODO: 개선 이후 제거 대상
        std::set<int>               mVertexDuplicationCheck;    // 폴리곤을 이용하여 모델을 구성하면 버텍스 중복이 생기므로 제거
        std::map<int, int>          mIndexMap;                  // 올바른 인덱스리스트 구성을 위한 PolygonVertex와 벡터와 연결
    };
}
