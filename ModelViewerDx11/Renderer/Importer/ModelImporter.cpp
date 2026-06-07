#include "ModelImporter.h"
#include <fstream>
#include "../Renderer.h"
#include "../../Util/Macro.h"

namespace renderer
{
    const wchar_t* ModelImporter::NORMAL_TEXTURE_FILE_SUFFIX = L"_N";
    const wchar_t* ModelImporter::TEXTURE_FILE_PATH = L"./AssetData/textures/";
    const wchar_t* ModelImporter::TEXTURE_FILE_EXTENSION = L".png";

    ModelImporter::ModelImporter()
        : mFbxManager(nullptr)
        , mImporter(nullptr)
        , mFbxScene(nullptr)
        , mSetting(nullptr)
        , mSumVertexCount(0)
        , mSumIndexCount(0)
        , mModelCenter(0.0f, 0.0f, 0.0f)
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

    void ModelImporter::LoadFbxModel(const char* fileName, Renderer* const renderer)
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

        parseTextureInfo(renderer);


        parseMaterial();


        mModelCenter /= mFbxObjects.size();


        mImporter->Destroy();
    }


    renderer::Mesh* ModelImporter::GetMesh(size_t meshIndex)
    {
        return &mFbxObjects[meshIndex].Mesh;
    }

    size_t ModelImporter::GetMeshCount() const
    {
        return mFbxObjects.size();
    }

    uint32 ModelImporter::GetSumVertexCount() const
    {
        return mSumVertexCount;
    }

    uint32 ModelImporter::GetSumIndexCount() const
    {
        return mSumIndexCount;
    }

    XMFLOAT3 ModelImporter::GetModelCenter() const
    {
        return XMFLOAT3(mModelCenter.mData[0], mModelCenter.mData[1], mModelCenter.mData[2]);
    }

    void ModelImporter::preprocess(FbxNode* parent, FbxNode* current)
    {
        if (current->GetCamera() || current->GetLight())
        {
            return;
        }

        MeshForImport newNode;
        ZeroMemory(&newNode, sizeof(MeshForImport));
        // 메시 타입인지? 메시를 가지고 있는 노드인지?
        // 메시에서 올바르게 데이터를 뽑아내기 위해서는
        // 우선 제대로 된 데이터를 가지고 있는 노드만을 선별해야 하므로
        // 거르는 것이 필요하다.
        if (current->GetMesh())
        {
            newNode.Current = current;
            // 현재 아직은 쓸모 없는데, 애니메이션을 위한 계층 구조 구성시 사용되지 않을까 예상.
            newNode.Parent = parent;

            FbxVector4 min;
            FbxVector4 max;
            FbxVector4 center;
            FbxTime time;
            current->EvaluateGlobalBoundingBoxMinMaxCenter(min, max, center, time);
            mModelCenter += center;

            bool bExist = false;
            for (MeshForImport node : mFbxObjects)
            {
                if (node.Current == current)
                {
                    bExist = true;
                }
            }

            if (!bExist)
            {
                mFbxObjects.push_back(newNode);
            }

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
            renderer::Mesh* const newMesh = &mFbxObjects[nodeIndex].Mesh; // meshData

            FbxMesh* currentMesh = mFbxObjects[nodeIndex].Current->GetMesh(); // mesh
            size_t numConverted = 0;
            mbstowcs_s(&numConverted, newMesh->Name, renderer::MESH_NAME_LENGTH, currentMesh->GetName(), strlen(currentMesh->GetName()));
            newMesh->Name[renderer::MESH_NAME_LENGTH - 1] = (WCHAR)L"\n";

            ASSERT(numConverted < renderer::MESH_NAME_LENGTH, "버퍼 초과")
                ASSERT(numConverted > 0, "복사된 문자가 없음.")


                // 메시가 가지는 모든 정점들의 위치 배열
                FbxVector4* allVertexPos = currentMesh->GetControlPoints();

            newMesh->IndexList.reserve(currentMesh->GetPolygonVertexCount());
            newMesh->Vertex.reserve(currentMesh->GetControlPointsCount());

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
                        if (mVertexDuplicationCheck.find(indexOfVertex) == mVertexDuplicationCheck.end())
                        {
                            mVertexDuplicationCheck.insert(indexOfVertex);

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

                            newMesh->Vertex.push_back(vertexInfo);
                            mIndexMap.insert(std::make_pair(indexOfVertex, newMesh->Vertex.size() - 1));

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
                        auto resultIter = mIndexMap.find(indexOfVertex);
                        if (resultIter == mIndexMap.end())
                        {
                            ASSERT(false, "map 구성 과정에 누락된 버텍스가 있음.");
                        }
                        newMesh->IndexList.push_back(resultIter->second);
                    }
                }
            }

            // 해당 메시가 가지는 버텍스 수를 알기 좋은 위치
            mSumVertexCount += newMesh->Vertex.size();
            mSumIndexCount += newMesh->IndexList.size();

            // 다음 메시를 세팅을 위한 초기화. 
            mVertexDuplicationCheck.clear();
            mIndexMap.clear();
        }
    }

    void ModelImporter::parseTextureInfo(Renderer* const renderer)
    {
        OutputDebugStringA("========== Texture Info Extraction start ==========\n");

        for (size_t objectIndex = 0; objectIndex < mFbxObjects.size(); ++objectIndex)
        {
            FbxNode* currentNode = mFbxObjects[objectIndex].Current;

            for (size_t materialIndex = 0; materialIndex < currentNode->GetSrcObjectCount<FbxSurfaceMaterial>(); ++materialIndex)
            {
                FbxSurfaceMaterial* material = currentNode->GetSrcObject<FbxSurfaceMaterial>(materialIndex);

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
                        FbxLayeredTexture* layeredTexture = FbxCast<FbxLayeredTexture>(property.GetSrcObject<FbxLayeredTexture>(layerIndex));

                        for (size_t textureIndex = 0; textureIndex < layeredTexture->GetSrcObjectCount<FbxTexture>(); ++textureIndex)
                        {
                            FbxFileTexture* texture = FbxCast<FbxFileTexture>(layeredTexture->GetSrcObject< FbxFileTexture>(textureIndex));
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

                    mFbxObjects[objectIndex].Mesh.NumTexuture = static_cast<uint8>(textureCount);

                    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
                    ZeroMemory(&desc, sizeof(desc));

                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    desc.Texture2D.MipLevels = 1;
                    desc.Texture2D.MostDetailedMip = 0;

                    for (int j = 0; j < textureCount; j++)
                    {
                        FbxFileTexture* texture = FbxCast<FbxFileTexture>(property.GetSrcObject<FbxFileTexture>(j));

                        // Then, you can get all the properties of the texture, include its name
                        if (texture)
                        {
                            OutputDebugStringA(texture->GetName());
                            OutputDebugStringA("\n");

                            enum { TEXTURE_NAME_LENGTH = 256 };

                            wchar_t textureFullPath[TEXTURE_NAME_LENGTH];
                            wchar_t textureName[TEXTURE_NAME_LENGTH];
                            //wchar_t normalTexturePath[TEXTURE_NAME_LENGTH];

                            //mbstowcs_s(&numConverted, textureName, TEXTURE_NAME_LENGTH, texture->GetName(), TEXTURE_NAME_LENGTH);

                            FbxString fileNameWithoutExtension = FbxPathUtils::GetFileName(texture->GetName(), false);

                            size_t numConverted = 0;
                            mbstowcs_s(&numConverted, textureName, TEXTURE_NAME_LENGTH, fileNameWithoutExtension.Buffer(), TEXTURE_NAME_LENGTH);

                            wsprintf(textureFullPath, L"%s%s%s", TEXTURE_FILE_PATH, textureName, TEXTURE_FILE_EXTENSION);

                            if (FAILED(loadTextureFromFileAndCreateResource(textureFullPath, renderer, desc, &mFbxObjects[objectIndex].Mesh.Texture)))
                            {
                                ASSERT(FALSE, "failed to create texture");
                            }

                            // for normal mapping
                            // face는 normal mapping용 텍스쳐가 존재하지 않음.
                            // sdk에서 normal texture를 뽑아낼 방법..
                            // TODO 이런 하드코드된 부분 처리할 필요 string 클래스를 만들어야 하나?
                            if (fileNameWithoutExtension.Compare("Unagi_Face_D") == 0)
                            {
                                mFbxObjects[objectIndex].Mesh.bLightMap = true;

                                continue;
                            }

                            ZeroMemory(textureFullPath, sizeof(wchar_t) * TEXTURE_NAME_LENGTH);
                            size_t nameLen = fileNameWithoutExtension.GetLen();
                            textureName[nameLen - 1] = NORMAL_TEXTURE_FILE_SUFFIX[1];
                            wsprintf(textureFullPath, L"%s%s%s", TEXTURE_FILE_PATH, textureName, TEXTURE_FILE_EXTENSION);

                            if (FAILED(loadTextureFromFileAndCreateResource(textureFullPath, renderer, desc, &mFbxObjects[objectIndex].Mesh.TextureNormal)))
                            {
                                ASSERT(FALSE, "failed to create texture for normal mapping");
                            }
                            mFbxObjects[objectIndex].Mesh.bLightMap = false;

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


        // TODO ligtmap 테스트 - 별도 셰이더를 제작하는 방법도 고려
        for (auto& mesh : mFbxObjects)
        {
            if (mesh.Mesh.bLightMap)
            {
                wchar_t textureFullPath[renderer::MESH_NAME_LENGTH];
                wsprintf(textureFullPath, L"%s%s%s", TEXTURE_FILE_PATH, L"Famale_Face_lightmap", TEXTURE_FILE_EXTENSION);
                D3D11_SHADER_RESOURCE_VIEW_DESC desc;
                ZeroMemory(&desc, sizeof(desc));

                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipLevels = 1;
                desc.Texture2D.MostDetailedMip = 0;
                if (FAILED(loadTextureFromFileAndCreateResource(textureFullPath, renderer, desc, &mesh.Mesh.TextureNormal)))
                {
                    ASSERT(FALSE, "failed to create texture for lightmapping");
                    return;
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

    void ModelImporter::parseMaterial()
    {


        /*   if (pGeometry) {
               lNode = pGeometry->GetNode();
               if (lNode)
                   lMaterialCount = lNode->GetMaterialCount();
           }*/
        for (size_t nodeIndex = 0; nodeIndex < mFbxObjects.size(); ++nodeIndex)
        {
            FbxNode* lNode = mFbxObjects[nodeIndex].Current;
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
                    DisplayString("            Name: \"", (char*)lMaterial->GetName(), "\"");
                    //Get the implementation to see if it's a hardware shader.
                    const FbxImplementation* lImplementation = LookForImplementation(lMaterial);
                    if (lImplementation)
                    {
                        //Now we have a hardware shader, let's read it
                        DisplayString("            Language: ", lImplementation->Language.Get().Buffer());
                        DisplayString("            LanguageVersion: ", lImplementation->LanguageVersion.Get().Buffer());
                        DisplayString("            RenderName: ", lImplementation->RenderName.Buffer());
                        DisplayString("            RenderAPI: ", lImplementation->RenderAPI.Get().Buffer());
                        DisplayString("            RenderAPIVersion: ", lImplementation->RenderAPIVersion.Get().Buffer());
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
                        mFbxObjects[nodeIndex].Mesh.Material.Diffuse.x = lKFbxDouble3.Get()[0];
                        mFbxObjects[nodeIndex].Mesh.Material.Diffuse.y = lKFbxDouble3.Get()[1];
                        mFbxObjects[nodeIndex].Mesh.Material.Diffuse.z = lKFbxDouble3.Get()[2];

                        lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Ambient;
                        mFbxObjects[nodeIndex].Mesh.Material.Ambient.x = lKFbxDouble3.Get()[0];
                        mFbxObjects[nodeIndex].Mesh.Material.Ambient.y = lKFbxDouble3.Get()[1];
                        mFbxObjects[nodeIndex].Mesh.Material.Ambient.z = lKFbxDouble3.Get()[2];

                        lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Specular;
                        mFbxObjects[nodeIndex].Mesh.Material.Specular.x = lKFbxDouble3.Get()[0];
                        mFbxObjects[nodeIndex].Mesh.Material.Specular.y = lKFbxDouble3.Get()[1];
                        mFbxObjects[nodeIndex].Mesh.Material.Specular.z = lKFbxDouble3.Get()[2];

                        lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Emissive;
                        mFbxObjects[nodeIndex].Mesh.Material.Emissive.x = lKFbxDouble3.Get()[0];
                        mFbxObjects[nodeIndex].Mesh.Material.Emissive.y = lKFbxDouble3.Get()[1];
                        mFbxObjects[nodeIndex].Mesh.Material.Emissive.z = lKFbxDouble3.Get()[2];

                        //Opacity is Transparency factor now
                        lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->TransparencyFactor;
                        mFbxObjects[nodeIndex].Mesh.Material.Opacity = 1.0 - lKFbxDouble1.Get();

                        lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->Shininess;
                        mFbxObjects[nodeIndex].Mesh.Material.Shininess = lKFbxDouble1.Get();

                        //// Display the Reflectivity
                        lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->ReflectionFactor;
                        mFbxObjects[nodeIndex].Mesh.Material.Reflectivity = lKFbxDouble1.Get();

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

    HRESULT ModelImporter::loadTextureFromFileAndCreateResource(
        const WCHAR*                           fileName,
        Renderer* const                        renderer,
        const D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc, ID3D11ShaderResourceView** outShaderResourceView)
    {
        ASSERT(*outShaderResourceView == nullptr, "nullptr가 아닌 ID3D11ShaderResourceView가 전달 되었습니다.");
        ID3D11Device* device = renderer->GetDevice();
        ScratchImage image;
        ID3D11Resource* textureResource = nullptr;
        HRESULT result = LoadFromWICFile(fileName, WIC_FLAGS_NONE, nullptr, image);
        if (FAILED(result))
        {
            OutputDebugStringW(L"ModelImporter::loadTextureFromFileAndCreateResource - 이미지 로드 실패");
            ASSERT(false, "이미지 로드 실패");
            goto FAILED;
        }

        result = CreateTexture(device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), &textureResource);
        if (FAILED(result))
        {
            OutputDebugStringW(L"ModelImporter::loadTextureFromFileAndCreateResource - CreateTexture 생성 실패");
            ASSERT(false, "CreateTexture gTextureResource 생성 실패");
            goto FAILED;
        }

        result = device->CreateShaderResourceView(textureResource, &srvDesc, &(*outShaderResourceView));
        if (FAILED(result))
        {
            ASSERT(false, "ModelImporter::loadTextureFromFileAndCreateResource - outShaderResourceView 생성 실패");
            goto FAILED;
        }

        result = S_OK;

    FAILED:
        image.Release();
        SAFETY_RELEASE(textureResource);
        SAFETY_RELEASE(device);

        return result;


    }
}
