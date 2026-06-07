#pragma once
#include "../Renderer.h"
#include "../Resources/ModelData.h"

namespace scene
{
    class Light;
    class Camera;
}

namespace renderer
{
    class ModelImporter;

    typedef renderer::Material CbMaterial;

    // 1. 셰이더 정리 (각 형식대로 파일 네이밍 변경 및 inputlayout등 코드 수정) - 완료
    // 2. Shader 매니저, Light 클래스, 텍스쳐 매니저 제작 - 완료(임시로 Renderer에)
    // TODO 3. 모델 로드할 수 있는 기능 추가
    // TODO 4. 모델 로드 및 텍스쳐 등 리소스 생성 실패시에도 돌 수 있도록 Default 리소스 준비 및 적용(기본 화면은 스카이박스 + 바닥면만 생성, 둘 중 안되면 단색상으로 초기화)

    class Model
    {
    public:
        Model(scene::Camera* camera, int8_t* filePath);
        ~Model();

        void                Draw();
        void                DrawShadow();

        void Update();

        // TODO: LightManager 만들면 제거.
        void SetLight(scene::Light* light);

        HRESULT             SetupMesh(ModelImporter& importer);

        void                SetHighlight(bool bSelection);

        size_t              GetMeshCount() const;
        const WCHAR* GetMeshName(size_t meshIndex) const;

        size_t              GetVertexCount(size_t meshIndex) const;
        size_t              GetIndexListCount(size_t meshIndex) const;

        XMFLOAT3            GetCenterPoint() const;
    private:


        void prepare();

    private:

        scene::Camera* mCamera; // 나중에 모델에 카메라를 붙이도록(상호참조해야 할 것 같음. 아니면 다른 방법)

        HashID mModelHash;
        size_t mNumMesh;
        size_t mNumVertex;

        // TODO: TextureManager 생성 시 이동해야 한다. Mesh 구조체 내부에 SRV를 가지고 있으므로 이것도 수정이 필요함.
        std::vector<renderer::Mesh> mMeshes;

        renderer::Vertex* mVertices;
        uint32* mIndices;


        XMFLOAT3 mCenterPosition;
        XMMATRIX mMatWorld;
        XMMATRIX mMatRotation;
        XMMATRIX mMatScale;

        scene::Light* mLight;



        bool mbHighlight;
        bool mbActiveEmissive;
    };
}

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
