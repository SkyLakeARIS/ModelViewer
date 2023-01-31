#include "pch.h"
#include "ModelImporter.h"

ModelImporter::ModelImporter()
    : mFbxScene(nullptr)
    , mFbxManager(nullptr)

{
    mFbxObjects.reserve(64);
}

ModelImporter::~ModelImporter()
{
    Release();
}

void ModelImporter::Initialize()
{
    mFbxManager = FbxManager::Create();

    mSetting = FbxIOSettings::Create(mFbxManager, IOSROOT);
    mFbxManager->SetIOSettings(mSetting);

    mImporter = FbxImporter::Create(mFbxManager, "myImporter");
}

void ModelImporter::Release()
{
    if (!mFbxScene)
    {
        mFbxScene->Destroy();
    }

    if (!mImporter)
    {
        mImporter->Destroy();
    }

    if (!mSetting)
    {
        mSetting->Destroy();
    }

    if (!mFbxManager)
    {
        mFbxManager->Destroy();
    }
}

void ModelImporter::LoadFbxModel(const char* fileName)
{

    if (!mImporter->Initialize(fileName, -1, mFbxManager->GetIOSettings()))
    {
        char str[128];
        sprintf_s(str, "\n\n\n\nERROR : %s\n\n\n\n", mImporter->GetStatus().GetErrorString());
        OutputDebugStringA(str);
        ASSERT(false, "mImporter->Initialize 실패.");
        return;
    }

    mFbxScene = FbxScene::Create(mFbxManager, "myScene");

    mImporter->Import(mFbxScene);


    FbxNode* rootNode = mFbxScene->GetRootNode();


    // 메시를 가진 노드들 추출
    preprocess(nullptr, rootNode);

    parseMesh();

    mImporter->Destroy();

}


const std::vector<Mesh>* ModelImporter::GetMesh() const
{
    return &mMeshes;
}

size_t ModelImporter::GetMeshCount() const
{
    return mMeshes.size();
}

void ModelImporter::preprocess(FbxNode* parent, FbxNode* current)
{
    if (current->GetCamera() || current->GetLight())
    {
        return;
    }

    ObjectNode node;
    // 메시 타입인지? 메시를 가지고 있는 노드인지?
    // 메시에서 올바르게 데이터를 뽑아내기 위해서는
    // 우선 제대로 된 데이터를 가지고 있는 노드만을 선별해야 하므로
    // 거르는 것이 필요하다.
    if (current->GetMesh())
    {
        node.current = current;
        // 현재 아직은 쓸모 없는데, 계층 구조 구성시 사용되지 않을까 예상.
        node.parent = parent;

        mFbxObjects.push_back(node);
    }

    size_t numChild = current->GetChildCount();
    for (size_t child = 0; child < numChild; ++child)
    {
        preprocess(current, current->GetChild(child));
    }
}

void ModelImporter::parseMesh()
{
    // 이전에 구성한 메시를 가지는 노드들을 순회.
    for (size_t nodeIndex = 0; nodeIndex < mFbxObjects.size(); ++nodeIndex)
    {
        Mesh meshTemp;
        mMeshes.push_back(meshTemp);
        Mesh& meshData = mMeshes.back();

        FbxMesh* mesh = mFbxObjects[nodeIndex].current->GetMesh();

        size_t numConverted = 0;
        mbstowcs_s(&numConverted, meshData.Name, MESH_NAME_LENGTH, mesh->GetName(), strlen(mesh->GetName()));
        meshData.Name[127] = (WCHAR)L"\n";

        ASSERT(numConverted < MESH_NAME_LENGTH, "버퍼 초과")
            ASSERT(numConverted > 0, "복사된 문자가 없음.")


            // 메시가 가지는 모든 정점들의 위치 배열
            FbxVector4* allVertexPos = mesh->GetControlPoints();

        meshData.IndexList.reserve(mesh->GetPolygonVertexCount());
        meshData.Vertex.reserve(mesh->GetControlPointsCount());

        // 메시가 가지는 폴리곤 수.
        size_t numPoly = mesh->GetPolygonCount();
        for (size_t polyIndex = 0; polyIndex < numPoly; ++polyIndex)
        {

            // 폴리곤의 구성단위 수(삼각형 || 사각형)
            size_t numVertexInPoly = mesh->GetPolygonSize(polyIndex);

            // 폴리곤을 구성하는 삼각형의 개수를 구함.
            // 폴리곤의 구성은 삼각형뿐만 아니라 사각형 등과 같은 형태도 될 수 있음.
            // 따라서 사각형인 경우(==삼각형 두개)도 대응할 수 있도록 한다.
            // 삼각형으로 분해한다고도 볼 수 있을 듯.
            size_t numFaceInPoly = numVertexInPoly - 2;
            for (size_t faceIndex = 0; faceIndex < numFaceInPoly; ++faceIndex)
            {
                // 이 부분. 참고 자료에는 max와 dx는 서로 다른 방향으로 인덱스가 배치된다 했는데,
                // 현재 가지고 있는 fbx파일은 어느 모델링 툴을 사용했는지 불분명하기 때문에 특정시켜 구현 불가.
                // 하지만 일단 구현은 참고자료와 동일하게 했다.
                // 만약에 파일에 좌표계의 형식이 기록되어 있고, sdk가 읽을 수 있다면 한번 해보는 것도 좋을 듯.
                // 참고: https://dlemrcnd.tistory.com/85
                size_t indexListOfTriangle[3] = { 0, faceIndex + 2, faceIndex + 1 };

                // indexListOfTriangle에 따른 삼각형 하나의 인덱스를 돌면서 정점의 필요한 정보들을 얻어온다.
                // 한번에 3개의 점 -> for문으로 하나씩
                int indexOfVertex = 0;
                FbxVector4 normalIndex;
                VertexInfo vertexInfo;
                for (size_t index = 0; index < 3; ++index)
                {
                    indexOfVertex = mesh->GetPolygonVertex(polyIndex, indexListOfTriangle[index]);

                    // 폴리곤 기준으로 진행하므로 인덱스 리스트를 사용하려면 중복처리를 해야한다.
                    // 이미 처리한 버텍스이면 건너뛴다.
                    if (mVertexDuplicationCheck.find(indexOfVertex) == mVertexDuplicationCheck.end())
                    {
                        mVertexDuplicationCheck.insert(indexOfVertex);

                        mesh->GetPolygonVertexNormal(polyIndex, indexListOfTriangle[index], normalIndex);

                        /*
                         *  position
                         */
                        FbxVector4 position = allVertexPos[indexOfVertex].mData;
                        vertexInfo.Pos.x = position[0];
                        vertexInfo.Pos.y = position[2];
                        vertexInfo.Pos.z = position[1];

                        /*
                         *  normal
                         */
                        FbxVector4 normal = normalIndex.mData;
                        vertexInfo.Norm.x = normal[0];
                        vertexInfo.Norm.y = normal[2];
                        vertexInfo.Norm.z = normal[1];

                        /*
                         *  texture
                         */
                        double* uv = nullptr;
                        // 텍스처를 가진 메시만
                        vertexInfo.Tex.x = 0.0f;
                        vertexInfo.Tex.y = 0.0f;
                        if (mesh->GetElementUVCount() > 0)
                        {
                            meshData.HasTexture = true;
                            FbxGeometryElementUV* texture = mesh->GetElementUV(0);
                            if (texture->GetMappingMode() == FbxGeometryElement::eByControlPoint
                                && texture->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                            {
                                size_t indexOfUV = texture->GetIndexArray().GetAt(indexOfVertex);
                                uv = texture->GetDirectArray().GetAt(indexOfVertex).mData;
                            }
                            else
                            {
                                uv = texture->GetDirectArray().GetAt(indexOfVertex).mData;
                            }

                            vertexInfo.Tex.x = uv[0];
                            vertexInfo.Tex.y = 1.0f - uv[1];
                        }

                        meshData.Vertex.push_back(vertexInfo);
                        mIndexMap.insert(std::make_pair(indexOfVertex, meshData.Vertex.size() - 1));

                    } // if end
                }

            }
        }  // object set end

        /*
        *  index list
        */
        for (size_t polyIndex = 0; polyIndex < numPoly; ++polyIndex)
        {
            size_t numVertexInPoly = mesh->GetPolygonSize(polyIndex);
            size_t numFaceInPoly = numVertexInPoly - 2;
            for (size_t faceIndex = 0; faceIndex < numFaceInPoly; ++faceIndex)
            {
                size_t indexListOfTriangle[3] = { 0, faceIndex + 2, faceIndex + 1 };

                // MAP과 비교하여 실제 버텍스에 따른 벡터내의 순서를 얻어와 인덱스 리스트 구성.
                for (size_t index = 0; index < 3; ++index)
                {
                    int indexOfVertex = mesh->GetPolygonVertex(polyIndex, indexListOfTriangle[index]);
                    auto resultIter = mIndexMap.find(indexOfVertex);
                    if (resultIter == mIndexMap.end())
                    {
                        ASSERT(false, "map 구성 과정에 누락된 버텍스가 있음.");
                    }
                    meshData.IndexList.push_back(resultIter->second);
                }
            }
        }

        // 다음 메시를 세팅을 위한 초기화. 
        mVertexDuplicationCheck.clear();
        mIndexMap.clear();
    }
}
