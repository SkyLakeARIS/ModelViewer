#include "ModelImporter.h"
#include <fstream>
#include <map>
#include <set>
#include "ImportedModelData.h"
#include "Shlwapi.h"
#include "../../Util/Define.h"
#include "../../Util/Macro.h"
#include "../../Util/Util.h"

namespace renderer
{

    const int8_t* const ModelImporter::TEXTURE_FILE_PATH_A = reinterpret_cast<const int8_t*>("./AssetData/textures/");
    const int8_t* const ModelImporter::TEXTURE_FILE_EXTENSION_A = reinterpret_cast<const int8_t*>(".png");

    ModelImporter::ModelImporter()
        : mFbxManager(nullptr)
        , mImporter(nullptr)
        , mFbxScene(nullptr)
        , mSetting(nullptr)
    {
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

    void ModelImporter::LoadFbxModel(const int8_t* const fileName, HashID& outModelHash, ImportedModelContainer& outModelContainer)
    {
        if (!mImporter->Initialize(reinterpret_cast<const char*>(fileName), -1, mFbxManager->GetIOSettings()))
        {
            int8_t str[util::MAX_STRING_LENGTH] = {};
            sprintf_s(reinterpret_cast<char*>(str), util::MAX_STRING_LENGTH, "\n\n\n\nERROR : %s\n\n\n\n", mImporter->GetStatus().GetErrorString());
            OutputDebugStringA(reinterpret_cast<LPCSTR>(str));
            ASSERT(false, "fbxImporter file initialization failed. fileName(%s)", fileName);
            return;
        }

        const HashID modelHash = util::GetDjb2Hash(fileName);
        ImportedModelContainer modelContainer = {};
        mFbxScene = FbxScene::Create(mFbxManager, "myScene");

        mImporter->Import(mFbxScene);

        FbxNode* rootNode = mFbxScene->GetRootNode();

        std::vector<FbxNode*> nodes;
        nodes.reserve(16);
        preprocess(nullptr, rootNode, nodes);

        modelContainer.Meshes.reserve(nodes.size());

        FbxVector4 minBound = {};
        FbxVector4 maxBound = {};
        FbxVector4 modelCenterPoint = {};
        parseMesh(nodes, modelContainer, minBound, maxBound);

        modelCenterPoint = (minBound + maxBound) * 0.5;
        modelContainer.CenterPoint.x = static_cast<float>(modelCenterPoint.mData[0]);
        modelContainer.CenterPoint.y = static_cast<float>(modelCenterPoint.mData[1]);
        modelContainer.CenterPoint.z = static_cast<float>(modelCenterPoint.mData[2]);
        modelContainer.CenterPoint.w = static_cast<float>(modelCenterPoint.mData[3]);

        modelContainer.ModelHash = modelHash;

        parseTextureInfo(nodes, modelContainer);

        parseMaterial(nodes, modelContainer);

        outModelContainer = std::move(modelContainer);
        outModelHash = modelHash;
        mImporter->Destroy();
    }

    void ModelImporter::preprocess(FbxNode* parent, FbxNode* current, std::vector<FbxNode*>& outNodes)
    {
        if (current->GetCamera() || current->GetLight())
        {
            return;
        }

        if (current->GetMesh())
        {
            bool bExist = false;
            for (const FbxNode* const node : outNodes)
            {
                if (node == current)
                {
                    bExist = true;
                }
            }

            if (!bExist)
            {
                outNodes.push_back(current);
            }
        }

        size_t numChild = current->GetChildCount();
        for (size_t child = 0; child < numChild; ++child)
        {
            preprocess(current, current->GetChild(child), outNodes);
        }
    }

    void ModelImporter::parseMesh(std::vector<FbxNode*>& outNodes, ImportedModelContainer& outModelContainer, FbxVector4& outMinBound, FbxVector4& outMaxBound)
    {
        std::set<int> vertexDuplicationCheck;
        std::map<int, int> indexMap;
        FbxVector4 minBound = {DBL_MAX, DBL_MAX , DBL_MAX , DBL_MAX };
        FbxVector4 maxBound = {DBL_MIN, DBL_MIN , DBL_MIN ,DBL_MIN };

        int32_t totalVertexCount = 0;
        int32_t totalIndexCount = 0;
        // calc total vtx/index count
        for (size_t nodeIndex = 0; nodeIndex < outNodes.size(); ++nodeIndex)
        {
            FbxMesh* currentMesh = outNodes[nodeIndex]->GetMesh(); // mesh

            const int32_t indexCount = currentMesh->GetPolygonVertexCount();
            ASSERT(indexCount > 0, "indexCount is not over 0  indexCount(%d)", indexCount);
            const int32_t vertexCount = currentMesh->GetControlPointsCount();
            ASSERT(vertexCount > 0, "vertexCount is not over 0  vertexCount(%d)", vertexCount);

            totalVertexCount += vertexCount;
            totalIndexCount += indexCount;
        }
        outModelContainer.TotalVertexCount = totalVertexCount;
        outModelContainer.TotalIndexCount = totalIndexCount;
        outModelContainer.IndexBufferTotal = std::make_unique<uint32_t[]>(totalIndexCount);
        outModelContainer.VertexBufferTotal = std::make_unique<Vertex[]>(totalVertexCount);

        int32_t vertexBufWriteCursor = 0;
        int32_t indexBufWriteCursor = 0;
        // 이전에 구성한 메시를 가지는 노드들을 순회.
        for (size_t nodeIndex = 0; nodeIndex < outNodes.size(); ++nodeIndex)
        {
            vertexDuplicationCheck.clear();
            indexMap.clear();
            FbxMesh* currentMesh = outNodes[nodeIndex]->GetMesh(); // mesh
            ImportedMeshData newMeshData = {};

            FbxVector4 minMeshBound;
            FbxVector4 maxMeshBound;
            FbxVector4 center;
            FbxTime time;
            outNodes[nodeIndex]->EvaluateGlobalBoundingBoxMinMaxCenter(minMeshBound, maxMeshBound, center, time);
            minBound[0] = std::min<double>(minBound[0], minMeshBound[0]);
            minBound[1] = std::min<double>(minBound[1], minMeshBound[2]);
            minBound[2] = std::min<double>(minBound[2], minMeshBound[1]);

            maxBound[0] = std::max<double>(maxBound[0], maxMeshBound[0]);
            maxBound[1] = std::max<double>(maxBound[1], maxMeshBound[2]);
            maxBound[2] = std::max<double>(maxBound[2], maxMeshBound[1]);
#ifdef _DEBUG
            size_t numConverted = 0;
            const int32_t meshNameLengthDebug = strlen(currentMesh->GetName());
            wchar_t* meshNameDebugOutput = new wchar_t[meshNameLengthDebug+1];

            (void)mbstowcs_s(&numConverted, meshNameDebugOutput, meshNameLengthDebug+1, currentMesh->GetName(), meshNameLengthDebug);

            OutputDebugStringW(meshNameDebugOutput);

            delete[] meshNameDebugOutput;
            meshNameDebugOutput = nullptr;

            ASSERT(numConverted <= meshNameLengthDebug + 1, "버퍼 초과");
            ASSERT(numConverted > 0, "복사된 문자가 없음.");
#endif

            // 메시가 가지는 모든 정점들의 위치 배열
            FbxVector4* allVertexPos = currentMesh->GetControlPoints();

            const int32_t indexCount = currentMesh->GetPolygonVertexCount();
            ASSERT(indexCount > 0, "indexCount is not over 0  indexCount(%d)", indexCount);
            const int32_t vertexCount = currentMesh->GetControlPointsCount();
            ASSERT(vertexCount > 0, "vertexCount is not over 0  vertexCount(%d)", vertexCount);
      
            // 메시가 가지는 폴리곤 수.
            size_t numPoly = currentMesh->GetPolygonCount();
            for (size_t polyIndex = 0; polyIndex < numPoly; ++polyIndex)
            {

                // 폴리곤의 구성단위 수(삼각형 || 사각형)
                size_t numVertexInPoly = currentMesh->GetPolygonSize(polyIndex);

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
                    renderer::Vertex vertexInfo;
                    for (size_t index = 0; index < 3; ++index)
                    {
                        indexOfVertex = currentMesh->GetPolygonVertex(polyIndex, indexListOfTriangle[index]);

                        // 폴리곤 기준으로 진행하므로 인덱스 리스트를 사용하려면 중복처리를 해야한다.
                        // 이미 처리한 버텍스이면 건너뛴다.
                        if (vertexDuplicationCheck.find(indexOfVertex) == vertexDuplicationCheck.end())
                        {
                            vertexDuplicationCheck.insert(indexOfVertex);

                            currentMesh->GetPolygonVertexNormal(polyIndex, indexListOfTriangle[index], normalIndex);

                            /*
                             *  position
                             */
                            FbxVector4 position = allVertexPos[indexOfVertex].mData;
                            vertexInfo.Position.x = position[0];
                            vertexInfo.Position.y = position[2];
                            vertexInfo.Position.z = position[1];

                            /*
                             *  normal
                             */
                            FbxVector4 normal = normalIndex.mData;
                            vertexInfo.Normal.x = normal[0];
                            vertexInfo.Normal.y = normal[2];
                            vertexInfo.Normal.z = normal[1];

                            /*
                             *  texture coord
                             */
                            double* uv = nullptr;
                            // 텍스처를 가진 메시만
                            vertexInfo.TexCoord.x = 0.0f;
                            vertexInfo.TexCoord.y = 0.0f;
                            if (currentMesh->GetElementUVCount() > 0)
                            {
                                FbxGeometryElementUV* texture = currentMesh->GetElementUV(0);
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

                                vertexInfo.TexCoord.x = uv[0];
                                vertexInfo.TexCoord.y = 1.0f - uv[1];
                            }
                            // TODO: improve - 이 부분은 개선하거나 데이터 구조를 좀 더 다듬는 게 좋아보인다. 메모해 두지 않으면 코드 다듬다가 실수하기 좋음.
                            // indexlist 구성을 위한 vertexCount와 통합 버퍼로 인한 CursorIndex 가 다르게 동작해야 하는 문제.
                            indexMap.insert(std::make_pair(indexOfVertex, newMeshData.VertexCount));
                            outModelContainer.VertexBufferTotal[vertexBufWriteCursor] = vertexInfo;
                            ++newMeshData.VertexCount;
                            ++vertexBufWriteCursor;
                        } // if end
                    }

                }
            }  // object set end

            /*
            *  index list
            */
            for (size_t polyIndex = 0; polyIndex < numPoly; ++polyIndex)
            {
                size_t numVertexInPoly = currentMesh->GetPolygonSize(polyIndex);
                size_t numFaceInPoly = numVertexInPoly - 2;
                for (size_t faceIndex = 0; faceIndex < numFaceInPoly; ++faceIndex)
                {
                    size_t indexListOfTriangle[3] = { 0, faceIndex + 2, faceIndex + 1 };

                    // MAP과 비교하여 실제 버텍스에 따른 벡터내의 순서를 얻어와 인덱스 리스트 구성.
                    for (size_t index = 0; index < 3; ++index)
                    {
                        int indexOfVertex = currentMesh->GetPolygonVertex(polyIndex, indexListOfTriangle[index]);
                        auto resultIter = indexMap.find(indexOfVertex);
                        if (resultIter == indexMap.end())
                        {
                            ASSERT(false, "map 구성 과정에 누락된 버텍스가 있음.");
                        }
                        outModelContainer.IndexBufferTotal[indexBufWriteCursor] = resultIter->second;
                        ++newMeshData.IndexCount;
                        ++indexBufWriteCursor;
                    }
                }
            }
            outModelContainer.Meshes.emplace_back(std::move(newMeshData));
        }
        outMinBound = minBound;
        outMaxBound = maxBound;
    }

    void ModelImporter::parseTextureInfo(std::vector<FbxNode*>& outNodes, ImportedModelContainer& outModelContainer)
    {
        OutputDebugStringA("========== Texture Info Extraction start ==========\n");

        // TODO: nodeIndex와 ImportedModelContainer의 mesh가 짝이 맞다고 보장 할 수 있는지? - 나중에 mesh와 짝지을 수 있는 수단으로 개선하면 좋을 것.
        for (size_t nodeIndex = 0; nodeIndex < outNodes.size(); ++nodeIndex)
        {
            FbxNode* const currentNode = outNodes[nodeIndex];

            for (size_t materialIndex = 0; materialIndex < currentNode->GetSrcObjectCount<FbxSurfaceMaterial>(); ++materialIndex)
            {
                FbxSurfaceMaterial* const material = currentNode->GetSrcObject<FbxSurfaceMaterial>(materialIndex);

                if (material == nullptr)
                {
                    OutputDebugStringA("material is nullptr");
                    continue;
                }

                FbxProperty property = material->FindProperty(FbxSurfaceMaterial::sDiffuse);

                size_t layerTextureCount = property.GetSrcObjectCount<FbxLayeredTexture>();
                if (layerTextureCount > 0)
                {
                    OutputDebugStringA("Get Texture from layered\n");

                    for (size_t layerIndex = 0; layerIndex < layerTextureCount; ++layerIndex)
                    {
                        FbxLayeredTexture* const layeredTexture = FbxCast<FbxLayeredTexture>(property.GetSrcObject<FbxLayeredTexture>(layerIndex));

                        for (size_t textureIndex = 0; textureIndex < layeredTexture->GetSrcObjectCount<FbxTexture>(); ++textureIndex)
                        {
                            FbxFileTexture* const texture = FbxCast<FbxFileTexture>(layeredTexture->GetSrcObject< FbxFileTexture>(textureIndex));
                            OutputDebugStringA(texture->GetName());
                            OutputDebugStringA("\n");
                        }
                    }
                }
                else
                {
                    OutputDebugStringA("Get Texture Direct\n");

                    const int textureCount = property.GetSrcObjectCount<FbxTexture>();
                    ASSERT(textureCount == 1, "다중 텍스처 대응 필요");

                    for (int j = 0; j < textureCount; j++)
                    {
                        FbxFileTexture* const texture = FbxCast<FbxFileTexture>(property.GetSrcObject<FbxFileTexture>(j));

                        if (texture)
                        {
                            OutputDebugStringA(texture->GetName());
                            OutputDebugStringA("\n");

                            FbxString fileNameWithoutExtension = FbxPathUtils::GetFileName(texture->GetName(), false);

                            // MEMO: 직접 작업하지 않는 이상 로드한 텍스쳐를 diffuse/normal/... 종류를 구분할 수 없음.
                            // 우선 가지고 있는 fbx 기준으로 하드코드한다.
                            if (fileNameWithoutExtension.Find("_D") > 0)
                            {
                                // TODO: improve - 파일이 있는지 없는지 체크 정도는 여기에서 하고 있는 경우에만 데이터를 구성하는 것도 좋을 것 같다.
                                // PathFileExistsA
                                outModelContainer.Meshes[nodeIndex].Textures[static_cast<int32_t>(eTextureType::Diffuse)].TextureType = eTextureType::Diffuse;
                                (void)sprintf_s(reinterpret_cast<char* const>(outModelContainer.Meshes[nodeIndex].Textures[static_cast<int32_t>(eTextureType::Diffuse)].FilePath), util::MAX_PATH_LENGTH, "%s%s%s", reinterpret_cast<const char*>(TEXTURE_FILE_PATH_A), fileNameWithoutExtension.Buffer(), reinterpret_cast<const char*>(TEXTURE_FILE_EXTENSION_A));

                                const HashID hash = util::GetDjb2Hash(outModelContainer.Meshes[nodeIndex].Textures[static_cast<int32_t>(eTextureType::Diffuse)].FilePath);
                                outModelContainer.Meshes[nodeIndex].Textures[static_cast<int32_t>(eTextureType::Diffuse)].TextureHash = hash;
                            }

                            // TODO: improve - Normal 텍스처를 하드코딩하여 일부 문자만 바꿔쓰는 방식으로 사용 중. Face는 Normal이 없으므로 이런 부분도 예외 처리가 필요
                            outModelContainer.Meshes[nodeIndex].Textures[static_cast<int32_t>(eTextureType::Normal)].TextureType = eTextureType::Normal;
                            (void)sprintf_s(reinterpret_cast<char* const>(outModelContainer.Meshes[nodeIndex].Textures[static_cast<int32_t>(eTextureType::Normal)].FilePath), util::MAX_PATH_LENGTH, "%s%s%s", reinterpret_cast<const char*>(TEXTURE_FILE_PATH_A), fileNameWithoutExtension.Buffer(), reinterpret_cast<const char*>(TEXTURE_FILE_EXTENSION_A));

                            const HashID hash = util::GetDjb2Hash(outModelContainer.Meshes[nodeIndex].Textures[static_cast<int32_t>(eTextureType::Normal)].FilePath);
                            outModelContainer.Meshes[nodeIndex].Textures[static_cast<int32_t>(eTextureType::Normal)].TextureHash = hash;
                        }
                        else
                        {
                            OutputDebugStringA("texture not found : set default texture");
                            OutputDebugStringA("\n");
                            ASSERT(false, "default texture에 대한 처리를 하지 않음.");
                        }

                        /* FbxProperty p = texture1->RootProperty.Find("Filename");
                         OutputDebugStringA(p.Get<FbxString>());*/
                    }
                }
            }
        }

        OutputDebugStringA("========== Texture Info Extraction end==========\n");
    }

    const FbxImplementation* ModelImporter::LookForImplementation(FbxSurfaceMaterial* pMaterial)
    {
        const FbxImplementation* lImplementation = nullptr;
        if (!lImplementation) lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_CGFX);
        if (!lImplementation) lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_HLSL);
        if (!lImplementation) lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_SFX);
        if (!lImplementation) lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_OGS);
        if (!lImplementation) lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_SSSL);
        return lImplementation;
    }

    void ModelImporter::parseMaterial(std::vector<FbxNode*>& outNodes, ImportedModelContainer& outModelContainer)
    {
        for (size_t nodeIndex = 0; nodeIndex < outNodes.size(); ++nodeIndex)
        {
            FbxNode* const lNode = outNodes[nodeIndex];
            int lMaterialCount = lNode->GetMaterialCount();

            if (lMaterialCount > 0)
            {
                FbxPropertyT<FbxDouble3> lKFbxDouble3;
                FbxPropertyT<FbxDouble> lKFbxDouble1;
                FbxColor theColor;
                for (int lCount = 0; lCount < lMaterialCount; lCount++)
                {
                    DisplayInt("        Material ", lCount);
                    FbxSurfaceMaterial* lMaterial = lNode->GetMaterial(lCount);
                    //Get the implementation to see if it's a hardware shader.
                    DisplayString("            Name: \"", (char*)lMaterial->GetName(), "\"");
                    const FbxImplementation* lImplementation = LookForImplementation(lMaterial);
                    if (lImplementation)
                    {
                        const FbxBindingTable* lRootTable = lImplementation->GetRootTable();
                        FbxString lFileName = lRootTable->DescAbsoluteURL.Get();
                        FbxString lTechniqueName = lRootTable->DescTAG.Get();
                        const FbxBindingTable* lTable = lImplementation->GetRootTable();
                        size_t lEntryNum = lTable->GetEntryCount();
                        for (int i = 0; i < (int)lEntryNum; ++i)
                        {
                            const FbxBindingTableEntry& lEntry = lTable->GetEntry(i);
                            const char* lEntrySrcType = lEntry.GetEntryType(true);
                            FbxProperty lFbxProp;
                            FbxString lTest = lEntry.GetSource();
                            DisplayString("            Entry: ", lTest.Buffer());
                            if (strcmp(FbxPropertyEntryView::sEntryType, lEntrySrcType) == 0)
                            {
                                lFbxProp = lMaterial->FindPropertyHierarchical(lEntry.GetSource());
                                if (!lFbxProp.IsValid())
                                {
                                    lFbxProp = lMaterial->RootProperty.FindHierarchical(lEntry.GetSource());
                                }
                            }
                            else if (strcmp(FbxConstantEntryView::sEntryType, lEntrySrcType) == 0)
                            {
                                lFbxProp = lImplementation->GetConstants().FindHierarchical(lEntry.GetSource());
                            }
                            if (lFbxProp.IsValid())
                            {
                                if (lFbxProp.GetSrcObjectCount<FbxTexture>() > 0)
                                {
                                    //do what you want with the textures
                                    for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxFileTexture>(); ++j)
                                    {
                                        FbxFileTexture* lTex = lFbxProp.GetSrcObject<FbxFileTexture>(j);
                                        DisplayString("           File Texture: ", lTex->GetFileName());
                                    }
                                    for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxLayeredTexture>(); ++j)
                                    {
                                        FbxLayeredTexture* lTex = lFbxProp.GetSrcObject<FbxLayeredTexture>(j);
                                        DisplayString("        Layered Texture: ", lTex->GetName());
                                    }
                                    for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxProceduralTexture>(); ++j)
                                    {
                                        FbxProceduralTexture* lTex = lFbxProp.GetSrcObject<FbxProceduralTexture>(j);
                                        DisplayString("     Procedural Texture: ", lTex->GetName());
                                    }
                                }
                                else
                                {
                                    FbxDataType lFbxType = lFbxProp.GetPropertyDataType();
                                    FbxString blah = lFbxType.GetName();
                                    if (FbxBoolDT == lFbxType)
                                    {
                                        DisplayBool("                Bool: ", lFbxProp.Get<FbxBool>());
                                    }
                                    else if (FbxIntDT == lFbxType || FbxEnumDT == lFbxType)
                                    {
                                        DisplayInt("                Int: ", lFbxProp.Get<FbxInt>());
                                    }
                                    else if (FbxFloatDT == lFbxType)
                                    {
                                        DisplayDouble("                Float: ", lFbxProp.Get<FbxFloat>());
                                    }
                                    else if (FbxDoubleDT == lFbxType)
                                    {
                                        DisplayDouble("                Double: ", lFbxProp.Get<FbxDouble>());
                                    }
                                    else if (FbxStringDT == lFbxType
                                        || FbxUrlDT == lFbxType
                                        || FbxXRefUrlDT == lFbxType)
                                    {
                                        DisplayString("                String: ", lFbxProp.Get<FbxString>().Buffer());
                                    }
                                    else if (FbxDouble2DT == lFbxType)
                                    {
                                        FbxDouble2 lDouble2 = lFbxProp.Get<FbxDouble2>();
                                        FbxVector2 lVect;
                                        lVect[0] = lDouble2[0];
                                        lVect[1] = lDouble2[1];
                                        Display2DVector("                2D vector: ", lVect);
                                    }
                                    else if (FbxDouble3DT == lFbxType || FbxColor3DT == lFbxType)
                                    {
                                        FbxDouble3 lDouble3 = lFbxProp.Get<FbxDouble3>();
                                        FbxVector4 lVect;
                                        lVect[0] = lDouble3[0];
                                        lVect[1] = lDouble3[1];
                                        lVect[2] = lDouble3[2];
                                        Display3DVector("                3D vector: ", lVect);
                                    }
                                    else if (FbxDouble4DT == lFbxType || FbxColor4DT == lFbxType)
                                    {
                                        FbxDouble4 lDouble4 = lFbxProp.Get<FbxDouble4>();
                                        FbxVector4 lVect;
                                        lVect[0] = lDouble4[0];
                                        lVect[1] = lDouble4[1];
                                        lVect[2] = lDouble4[2];
                                        lVect[3] = lDouble4[3];
                                        Display4DVector("                4D vector: ", lVect);
                                    }
                                    else if (FbxDouble4x4DT == lFbxType)
                                    {
                                        FbxDouble4x4 lDouble44 = lFbxProp.Get<FbxDouble4x4>();
                                        for (int j = 0; j < 4; ++j)
                                        {
                                            FbxVector4 lVect;
                                            lVect[0] = lDouble44[j][0];
                                            lVect[1] = lDouble44[j][1];
                                            lVect[2] = lDouble44[j][2];
                                            lVect[3] = lDouble44[j][3];
                                            Display4DVector("                4x4D vector: ", lVect);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (lMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
                    {
                        // We found a Phong material.  Display its properties.
                        // Display the Ambient Color
                        lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Diffuse;
                        outModelContainer.Meshes[nodeIndex].Material.Diffuse.x = lKFbxDouble3.Get()[0];
                        outModelContainer.Meshes[nodeIndex].Material.Diffuse.y = lKFbxDouble3.Get()[1];
                        outModelContainer.Meshes[nodeIndex].Material.Diffuse.z = lKFbxDouble3.Get()[2];

                        lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Ambient;
                        outModelContainer.Meshes[nodeIndex].Material.Ambient.x = lKFbxDouble3.Get()[0];
                        outModelContainer.Meshes[nodeIndex].Material.Ambient.y = lKFbxDouble3.Get()[1];
                        outModelContainer.Meshes[nodeIndex].Material.Ambient.z = lKFbxDouble3.Get()[2];

                        lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Specular;
                        outModelContainer.Meshes[nodeIndex].Material.Specular.x = lKFbxDouble3.Get()[0];
                        outModelContainer.Meshes[nodeIndex].Material.Specular.y = lKFbxDouble3.Get()[1];
                        outModelContainer.Meshes[nodeIndex].Material.Specular.z = lKFbxDouble3.Get()[2];

                        lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Emissive;
                        outModelContainer.Meshes[nodeIndex].Material.Emissive.x = lKFbxDouble3.Get()[0];
                        outModelContainer.Meshes[nodeIndex].Material.Emissive.y = lKFbxDouble3.Get()[1];
                        outModelContainer.Meshes[nodeIndex].Material.Emissive.z = lKFbxDouble3.Get()[2];

                        //Opacity is Transparency factor now
                        lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->TransparencyFactor;
                        outModelContainer.Meshes[nodeIndex].Material.Opacity = 1.0 - lKFbxDouble1.Get();

                        lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->Shininess;
                        outModelContainer.Meshes[nodeIndex].Material.Shininess = lKFbxDouble1.Get();

                        //// Display the Reflectivity
                        lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->ReflectionFactor;
                        outModelContainer.Meshes[nodeIndex].Material.Reflectivity = lKFbxDouble1.Get();

                    }
                    else if (lMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
                    {
                        // We found a Lambert material. Display its properties.
                        // Display the Ambient Color
                        lKFbxDouble3 = ((FbxSurfaceLambert*)lMaterial)->Ambient;
                        theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                        DisplayColor("            Ambient: ", theColor);
                        // Display the Diffuse Color
                        lKFbxDouble3 = ((FbxSurfaceLambert*)lMaterial)->Diffuse;
                        theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                        DisplayColor("            Diffuse: ", theColor);
                        // Display the Emissive
                        lKFbxDouble3 = ((FbxSurfaceLambert*)lMaterial)->Emissive;
                        theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                        DisplayColor("            Emissive: ", theColor);
                        // Display the Opacity
                        lKFbxDouble1 = ((FbxSurfaceLambert*)lMaterial)->TransparencyFactor;
                        DisplayDouble("            Opacity: ", 1.0 - lKFbxDouble1.Get());
                    }
                    else
                        DisplayString("Unknown type of Material");
                    FbxPropertyT<FbxString> lString;
                    lString = lMaterial->ShadingModel;
                    DisplayString("            Shading Model: ", lString.Get().Buffer());
                    DisplayString("");
                }
            }
        }
    }

    void ModelImporter::DisplayString(const char* pHeader, const char* pValue /* = "" */, const char* pSuffix /* = "" */)
    {
        FbxString lString;
        lString = pHeader;
        lString += pValue;
        lString += pSuffix;
        lString += "\n";
        PrintString(lString);
    }

    void ModelImporter::DisplayBool(const char* pHeader, bool pValue, const char* pSuffix /* = "" */)
    {
        FbxString lString;
        lString = pHeader;
        lString += pValue ? "true" : "false";
        lString += pSuffix;
        lString += "\n";
        PrintString(lString);
    }

    void ModelImporter::DisplayInt(const char* pHeader, int pValue, const char* pSuffix /* = "" */)
    {
        FbxString lString;
        lString = pHeader;
        lString += pValue;
        lString += pSuffix;
        lString += "\n";
        PrintString(lString);
    }

    void ModelImporter::DisplayDouble(const char* pHeader, double pValue, const char* pSuffix /* = "" */)
    {
        FbxString lString;
        FbxString lFloatValue = (float)pValue;
        lFloatValue = pValue <= -HUGE_VAL ? "-INFINITY" : lFloatValue.Buffer();
        lFloatValue = pValue >= HUGE_VAL ? "INFINITY" : lFloatValue.Buffer();
        lString = pHeader;
        lString += lFloatValue;
        lString += pSuffix;
        lString += "\n";
        PrintString(lString);
    }

    void ModelImporter::Display2DVector(const char* pHeader, FbxVector2 pValue, const char* pSuffix  /* = "" */)
    {
        FbxString lString;
        FbxString lFloatValue1 = (float)pValue[0];
        FbxString lFloatValue2 = (float)pValue[1];
        lFloatValue1 = pValue[0] <= -HUGE_VAL ? "-INFINITY" : lFloatValue1.Buffer();
        lFloatValue1 = pValue[0] >= HUGE_VAL ? "INFINITY" : lFloatValue1.Buffer();
        lFloatValue2 = pValue[1] <= -HUGE_VAL ? "-INFINITY" : lFloatValue2.Buffer();
        lFloatValue2 = pValue[1] >= HUGE_VAL ? "INFINITY" : lFloatValue2.Buffer();
        lString = pHeader;
        lString += lFloatValue1;
        lString += ", ";
        lString += lFloatValue2;
        lString += pSuffix;
        lString += "\n";
        PrintString(lString);
    }

    void ModelImporter::Display3DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix /* = "" */)
    {
        FbxString lString;
        FbxString lFloatValue1 = (float)pValue[0];
        FbxString lFloatValue2 = (float)pValue[1];
        FbxString lFloatValue3 = (float)pValue[2];
        lFloatValue1 = pValue[0] <= -HUGE_VAL ? "-INFINITY" : lFloatValue1.Buffer();
        lFloatValue1 = pValue[0] >= HUGE_VAL ? "INFINITY" : lFloatValue1.Buffer();
        lFloatValue2 = pValue[1] <= -HUGE_VAL ? "-INFINITY" : lFloatValue2.Buffer();
        lFloatValue2 = pValue[1] >= HUGE_VAL ? "INFINITY" : lFloatValue2.Buffer();
        lFloatValue3 = pValue[2] <= -HUGE_VAL ? "-INFINITY" : lFloatValue3.Buffer();
        lFloatValue3 = pValue[2] >= HUGE_VAL ? "INFINITY" : lFloatValue3.Buffer();
        lString = pHeader;
        lString += lFloatValue1;
        lString += ", ";
        lString += lFloatValue2;
        lString += ", ";
        lString += lFloatValue3;
        lString += pSuffix;
        lString += "\n";
        PrintString(lString);
    }

    void ModelImporter::Display4DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix /* = "" */)
    {
        FbxString lString;
        FbxString lFloatValue1 = (float)pValue[0];
        FbxString lFloatValue2 = (float)pValue[1];
        FbxString lFloatValue3 = (float)pValue[2];
        FbxString lFloatValue4 = (float)pValue[3];
        lFloatValue1 = pValue[0] <= -HUGE_VAL ? "-INFINITY" : lFloatValue1.Buffer();
        lFloatValue1 = pValue[0] >= HUGE_VAL ? "INFINITY" : lFloatValue1.Buffer();
        lFloatValue2 = pValue[1] <= -HUGE_VAL ? "-INFINITY" : lFloatValue2.Buffer();
        lFloatValue2 = pValue[1] >= HUGE_VAL ? "INFINITY" : lFloatValue2.Buffer();
        lFloatValue3 = pValue[2] <= -HUGE_VAL ? "-INFINITY" : lFloatValue3.Buffer();
        lFloatValue3 = pValue[2] >= HUGE_VAL ? "INFINITY" : lFloatValue3.Buffer();
        lFloatValue4 = pValue[3] <= -HUGE_VAL ? "-INFINITY" : lFloatValue4.Buffer();
        lFloatValue4 = pValue[3] >= HUGE_VAL ? "INFINITY" : lFloatValue4.Buffer();
        lString = pHeader;
        lString += lFloatValue1;
        lString += ", ";
        lString += lFloatValue2;
        lString += ", ";
        lString += lFloatValue3;
        lString += ", ";
        lString += lFloatValue4;
        lString += pSuffix;
        lString += "\n";
        PrintString(lString);
    }

    void ModelImporter::DisplayColor(const char* pHeader, FbxColor pValue, const char* pSuffix /* = "" */)
    {
        FbxString lString;
        lString = pHeader;
        lString += (float)pValue.mRed;
        lString += " (red), ";
        lString += (float)pValue.mGreen;
        lString += " (green), ";
        lString += (float)pValue.mBlue;
        lString += " (blue)";
        lString += pSuffix;
        lString += "\n";
        PrintString(lString);
    }

    void ModelImporter::PrintString(FbxString& pString)
    {
        bool lReplaced = pString.ReplaceAll("%", "%%");
        FBX_ASSERT(lReplaced == false);
        OutputDebugStringA(pString.Buffer());
    }
}
