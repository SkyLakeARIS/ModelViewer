#include "pch.h"
#include "ModelImporter.h"
#include <fstream>
#include <string>

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

void ModelImporter::LoadFBXAndCreateModel(const char* fileName, Model** model)
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

    *model = new Model();

    parseMesh(*(*model)->GetMeshListReference());


    mImporter->Destroy();

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

void ModelImporter::parseMesh(std::vector<Mesh>& meshesToLoad)
{
    // 이전에 구성한 메시를 가지는 노드들을 순회.
    for (size_t nodeIndex = 0; nodeIndex < mFbxObjects.size(); ++nodeIndex)
    {
        Mesh meshTemp;
        meshesToLoad.push_back(meshTemp);
        Mesh& meshData = meshesToLoad.back();

        FbxMesh* mesh = mFbxObjects[nodeIndex].current->GetMesh();

        size_t numConverted = 0;

        mbstowcs_s(&numConverted, meshData.Name, 128, mesh->GetName(), strlen(mesh->GetName()));
        ASSERT(numConverted < 128, "버퍼 초과")
            ASSERT(numConverted > 0, "복사된 문자가 없음.")

            meshData.Name[127] = (wchar_t)L"\n";

        // 메시가 가지는 모든 정점들의 위치 배열
        FbxVector4* allVertexPos = mesh->GetControlPoints();
        meshData.Vertex.reserve(mesh->GetControlPointsCount());

        // 메시가 가지는 폴리곤 수.
        size_t numPoly = mesh->GetPolygonCount();
        for (size_t polyIndex = 0; polyIndex < numPoly; ++polyIndex)
        {
            // 폴리곤의 구성단위 수(삼각형 || 사각형)
            size_t numVertexInPoly = mesh->GetPolygonSize(polyIndex);
            // 
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
                size_t indexListOfTriangle[3] = { 0, faceIndex + 2, faceIndex + 1 };

                // indexListOfTriangle에 따른 삼각형 하나의 인덱스를 돌면서 정점의 필요한 정보들을 얻어온다.
                // 한번에 3개의 점 -> for문으로 하나씩
                int indexOfVertex;
                FbxVector4 normalIndex;
                VertexInfo vertexInfo;
                for (size_t index = 0; index < 3; ++index)
                {
                    indexOfVertex = mesh->GetPolygonVertex(polyIndex, indexListOfTriangle[index]);
                    mesh->GetPolygonVertexNormal(polyIndex, indexListOfTriangle[index], normalIndex);

                    FbxVector4 position = allVertexPos[indexOfVertex].mData;
                    vertexInfo.Pos.x = position[0];
                    vertexInfo.Pos.y = position[2];
                    vertexInfo.Pos.z = position[1];

                    FbxVector4 normal = normalIndex.mData;
                    vertexInfo.Norm.x = normal[0];
                    vertexInfo.Norm.y = normal[2];
                    vertexInfo.Norm.z = normal[1];

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

                }

            }
        }
    }
}




/*
 *  sdk sample code
 */
void ModelImporter::printNode(FbxNode* node)
{

    const char* nodeName = node->GetName();
    FbxDouble3 translation = node->LclTranslation.Get();
    FbxDouble3 rotation = node->LclRotation.Get();
    FbxDouble3 scaling = node->LclScaling.Get();

    char str[256];
    sprintf_s(str, "<node name='%s' translation='(%lf, %lf, %lf)' rotation='(%lf, %lf, %lf)' scaling='(%lf, %lf, %lf)'>\n",
        nodeName,
        translation[0], translation[1], translation[2],
        rotation[0], rotation[1], rotation[2],
        scaling[0], scaling[1], scaling[2]);

    OutputDebugStringA(str);

    // Print the node's attributes.
    for (int i = 0; i < node->GetNodeAttributeCount(); i++)
        printAttribute(node->GetNodeAttributeByIndex(i));

    // Recursively print the children.
    for (int j = 0; j < node->GetChildCount(); j++)
        printNode(node->GetChild(j));

    OutputDebugStringA("</node>\n");
}

/**
 *
 *  sdk sample code
 *
* Return a string-based representation based on the attribute type.
*/
FbxString ModelImporter::getAttributeTypeName(FbxNodeAttribute::EType type) {
    switch (type) {
    case FbxNodeAttribute::eUnknown: return "unidentified";
    case FbxNodeAttribute::eNull: return "null";
    case FbxNodeAttribute::eMarker: return "marker";
    case FbxNodeAttribute::eSkeleton: return "skeleton";
    case FbxNodeAttribute::eMesh: return "mesh";
    case FbxNodeAttribute::eNurbs: return "nurbs";
    case FbxNodeAttribute::ePatch: return "patch";
    case FbxNodeAttribute::eCamera: return "camera";
    case FbxNodeAttribute::eCameraStereo: return "stereo";
    case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
    case FbxNodeAttribute::eLight: return "light";
    case FbxNodeAttribute::eOpticalReference: return "optical reference";
    case FbxNodeAttribute::eOpticalMarker: return "marker";
    case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
    case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
    case FbxNodeAttribute::eBoundary: return "boundary";
    case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
    case FbxNodeAttribute::eShape: return "shape";
    case FbxNodeAttribute::eLODGroup: return "lodgroup";
    case FbxNodeAttribute::eSubDiv: return "subdiv";
    default: return "unknown";
    }
}

/**
 *
 *  sdk sample code
 * Print an attribute.
 */
void ModelImporter::printAttribute(FbxNodeAttribute* pAttribute) {
    if (!pAttribute) return;

    FbxString typeName = getAttributeTypeName(pAttribute->GetAttributeType());
    FbxString attrName = pAttribute->GetName();

    // Note: to retrieve the character array of a FbxString, use its Buffer() method.
    char str[256];
    sprintf_s(str, "\n<attribute type='%s' name='%s'/>\n", typeName.Buffer(), attrName.Buffer());
    OutputDebugStringA(str);

}
