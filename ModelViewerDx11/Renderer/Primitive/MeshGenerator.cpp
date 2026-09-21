#include "MeshGenerator.h"
#include <memory>
#include "../../Util/Macro.h"
#include "../../Util/Util.h"
#include "../Resources/BufferManager.h"
#include "../Resources/Mesh.h"

namespace renderer
{
    const int8_t* const MeshGenerator::VIRTUAL_ROOT_PATH = reinterpret_cast<const int8_t*>("/AssetData/Generated/");
    BufferManager* MeshGenerator::sBufferManager = nullptr;

    void MeshGenerator::Initialize(BufferManager* const bufferManager)
    {
        ASSERT(bufferManager, "bufferManager is nullptr.");
        ASSERT(!sBufferManager, "initialize has been invoked twice.");
        sBufferManager = bufferManager;
    }


    // from: https://www.braynzarsoft.net/viewtutorial/q16390-20-cube-mapping-skybox
    // fix: changed cap indices winding to CW for consistency.
    void MeshGenerator::CreateSphere(uint16_t latLines, uint16_t lonLines, Mesh& outMesh)
    {
        ASSERT(sBufferManager, "MeshGenerator not initialized. (Call the Initialize)");
        ASSERT(latLines > 0, "latLines too small. (line > 0)");
        ASSERT(lonLines > 0, "lonLines too small. (line > 0)");

        const uint32_t numVertex = ((latLines - 2) * lonLines) + 2;
        const uint32_t numFace = ((latLines - 3) * (lonLines) * 2) + (lonLines * 2);

        std::vector<VertexP> vertices(numVertex);

        vertices[0].Position.x = 0.0f;
        vertices[0].Position.y = 0.0f;
        vertices[0].Position.z = 1.0f;

        XMMATRIX matRotationX;
        XMMATRIX matRotationY;
        float sphereYaw = 0.0f;
        float spherePitch = 0.0f;
        XMVECTOR currVertPos = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        for (uint32_t i = 0; i < latLines - 2U; ++i)
        {
            spherePitch = (float)(i + 1U) * (3.14f / (float)(latLines - 1U));
            matRotationX = XMMatrixRotationX(spherePitch);
            for (uint32_t j = 0U; j < lonLines; ++j)
            {
                sphereYaw = (float)j * (6.28f / (float)lonLines);
                matRotationY = XMMatrixRotationZ(sphereYaw);
                currVertPos = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (matRotationX * matRotationY));
                currVertPos = XMVector3Normalize(currVertPos);
                uint32_t index = i * lonLines + j + 1U;
                vertices[index].Position.x = XMVectorGetX(currVertPos);
                vertices[index].Position.y = XMVectorGetY(currVertPos);
                vertices[index].Position.z = XMVectorGetZ(currVertPos);
            }
        }

        vertices[numVertex - 1U].Position.x = 0.0f;
        vertices[numVertex - 1U].Position.y = 0.0f;
        vertices[numVertex - 1U].Position.z = -1.0f;

        std::vector<uint32_t> indices(numFace * 3U);

        int k = 0;
        for (uint32_t l = 0U; l < lonLines - 1U; ++l)
        {
            indices[k] = l + 2U;
            indices[k + 1] = l + 1U;
            indices[k + 2] = 0U;
            k += 3;
        }

        indices[k] = 1U;
        indices[k + 1] = lonLines;
        indices[k + 2] = 0U;
        k += 3;

        for (uint32_t i = 0U; i < latLines - 3U; ++i)
        {
            for (uint32_t j = 0U; j < lonLines - 1U; ++j)
            {
                indices[k] = i * lonLines + j + 1U;
                indices[k + 1] = i * lonLines + j + 2U;
                indices[k + 2] = (i + 1U) * lonLines + j + 1U;

                indices[k + 3] = (i + 1U) * lonLines + j + 1U;
                indices[k + 4] = i * lonLines + j + 2U;
                indices[k + 5] = (i + 1U) * lonLines + j + 2U;

                k += 6; // next quad
            }

            indices[k] = (i * lonLines) + lonLines;
            indices[k + 1] = (i * lonLines) + 1U;
            indices[k + 2] = ((i + 1U) * lonLines) + lonLines;

            indices[k + 3] = ((i + 1U) * lonLines) + lonLines;
            indices[k + 4] = (i * lonLines) + 1U;
            indices[k + 5] = ((i + 1U) * lonLines) + 1U;

            k += 6;
        }

        for (uint32_t l = 0U; l < lonLines - 1U; ++l)
        {
            indices[k] = (numVertex - 1U) - (l + 2U);
            indices[k + 1] = (numVertex - 1U) - (l + 1U);
            indices[k + 2] = numVertex - 1U;
            k += 3;
        }

        indices[k] = numVertex - 2U;
        indices[k + 1] = (numVertex - 1U) - lonLines;
        indices[k + 2] = numVertex - 1U;


        outMesh.VertexFormat = eVertexFormat::P;
        SubMesh newSubMesh = {};

        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        const int32_t pathLength = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Sphere_%d_%d.mesh", reinterpret_cast<const char*>(VIRTUAL_ROOT_PATH), latLines, lonLines);
        ASSERT(pathLength < util::MAX_PATH_LENGTH, "file path too long. length(%d), limit(%d)", pathLength, util::MAX_PATH_LENGTH);

        (void)memcpy(outMesh.MeshName, virtualFilePath, pathLength + 1);
        (void)memcpy(newSubMesh.SubMeshName, virtualFilePath, pathLength + 1);
        outMesh.MeshHash = util::GetDjb2Hash(virtualFilePath);
        newSubMesh.SubMeshHash = outMesh.MeshHash;

        const int16_t strideVertex = GetVertexStrideSize(outMesh.VertexFormat);
        const int16_t strideIndex = sBufferManager->GetIndexStrideSize();

        sBufferManager->AddVertex(reinterpret_cast<int8_t*>(vertices.data()), strideVertex * vertices.size(), newSubMesh.SubMeshHash, strideVertex, newSubMesh.VertexRange);
        sBufferManager->AddIndex(reinterpret_cast<int8_t*>(indices.data()), strideIndex * indices.size(), newSubMesh.SubMeshHash, strideIndex, newSubMesh.IndexRange);

        outMesh.VertexRange = newSubMesh.VertexRange;
        outMesh.IndexRange = newSubMesh.IndexRange;

        outMesh.SubMeshes.push_back(std::move(newSubMesh));
    }

    void MeshGenerator::CreateGrid(XMFLOAT2 startPoint, uint16_t horizontalLines, uint16_t verticalLines, float gapEachLine, Mesh& outMesh)
    {
        ASSERT(sBufferManager, "MeshGenerator not initialized. (Call the Initialize)");
        ASSERT(horizontalLines >= 2, "horizontalLines must be 2 or greater. passed(%d)", horizontalLines);
        ASSERT(verticalLines >= 2, "verticalLines must be 2 or greater. passed(%d)", verticalLines);
        ASSERT(gapEachLine >= 1, "gapEachLine must be 1 or greater");

        const uint32_t numRows = horizontalLines - 1;
        const int32_t numVertices = (verticalLines * 2) * numRows;
        const int32_t numIndices = numRows * (verticalLines - 1) * 6;

        std::unique_ptr<VertexPTN[]> vertices = std::make_unique<VertexPTN[]>(numVertices);
        std::unique_ptr<uint32_t[]> indices = std::make_unique<uint32_t[]>(numIndices);

        VertexPTN* curVertices = vertices.get();
        XMFLOAT2  pos(startPoint);
        for (uint32_t lineY = 0; lineY < horizontalLines - 1; ++lineY)
        {
            for (uint32_t lineX = 0; lineX < verticalLines; ++lineX)
            {
                // triangle-strip
                curVertices->Position = XMFLOAT3(pos.x, 0.0f, pos.y);
                curVertices->TexCoord = XMFLOAT2(0.0f, 0.0f);
                ++curVertices;
                curVertices->Position = XMFLOAT3(pos.x, 0.0f, pos.y + gapEachLine);
                curVertices->TexCoord = XMFLOAT2(0.0f, 0.0f);
                ++curVertices;
                pos.x += gapEachLine;
            }
            if (lineY < numRows - 1)
            {
                pos.x = startPoint.x;
                pos.y += gapEachLine;
            }
        }

        // MEMO: IndexList 생성 부분
        uint32_t* curIndices = indices.get();
        const uint32_t numVertexEachLine = (verticalLines * 2);
        for(uint32_t vertexIndex = 0; vertexIndex < numVertices -1; ++vertexIndex)
        {
            if(vertexIndex & 0x01)
            {
                *curIndices = vertexIndex;
                ++curIndices;
                *curIndices = vertexIndex + 2;
                ++curIndices;
                *curIndices = vertexIndex + 1;
                ++curIndices;
            }
            else
            {
                *curIndices = vertexIndex;
                ++curIndices;
                *curIndices = vertexIndex + 1;
                ++curIndices;
                *curIndices = vertexIndex + 2;
                ++curIndices;
            }

            if ((vertexIndex + 3) % numVertexEachLine == 0)
            {
                vertexIndex += 2;
            }
        }

        // MEMO: Normal 데이터 생성 부분
        // MEMO: 한 평면 위의 점들이니 전부 같은 Normal로 세팅한다.
        // MEMO: 규칙: winding이 시계방향이므로, 노멀도 왼손감기 기준으로 생성
        curVertices = vertices.get();

        XMVECTOR point1; // MEMO: pivot
        XMVECTOR point2;
        XMVECTOR point3;
        point1 = XMLoadFloat3(&(curVertices)->Position);
        point2 = XMLoadFloat3(&(curVertices + 1)->Position);
        point3 = XMLoadFloat3(&(curVertices + 2)->Position);

        XMVECTOR vP1ToP2 = XMVectorSubtract(point2, point1);
        XMVECTOR vP1ToP3 = XMVectorSubtract(point3, point1);
        XMVECTOR vNormal = XMVector3Cross(vP1ToP2, vP1ToP3);
        vNormal = XMVector3Normalize(vNormal);
        for (uint32_t index = 0; index < numVertices; ++index)
        {
            XMStoreFloat3(&(curVertices + index)->Normal, vNormal);
        }

        outMesh.VertexFormat = eVertexFormat::PTN;
        const int16_t strideVertex = GetVertexStrideSize(outMesh.VertexFormat);
        SubMesh newSubMesh = {};
        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        const int32_t pathLength = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Grid_%d_%d.mesh", reinterpret_cast<const char*>(VIRTUAL_ROOT_PATH), verticalLines, horizontalLines);
        ASSERT(pathLength < util::MAX_PATH_LENGTH, "file path too long. length(%d), limit(%d)", pathLength, util::MAX_PATH_LENGTH);

        (void)memcpy(outMesh.MeshName, virtualFilePath, pathLength + 1);
        (void)memcpy(newSubMesh.SubMeshName, virtualFilePath, pathLength + 1);
        outMesh.MeshHash = util::GetDjb2Hash(virtualFilePath);
        newSubMesh.SubMeshHash = outMesh.MeshHash;


        sBufferManager->AddVertex(reinterpret_cast<int8_t*>(vertices.get()), strideVertex * numVertices, newSubMesh.SubMeshHash, strideVertex, newSubMesh.VertexRange);

        const int16_t strideIndex = sBufferManager->GetIndexStrideSize();
        sBufferManager->AddIndex(reinterpret_cast<int8_t*>(indices.get()), strideIndex * numIndices, newSubMesh.SubMeshHash, strideIndex, newSubMesh.IndexRange);

        outMesh.VertexRange = newSubMesh.VertexRange;
        outMesh.IndexRange = newSubMesh.IndexRange;

        newSubMesh.Material.Factors.IsLitOn = static_cast<float>(true);

        outMesh.SubMeshes.push_back(std::move(newSubMesh));
    }

    void MeshGenerator::CreatePlane(Mesh& outMesh)
    {
        ASSERT(sBufferManager, "MeshGenerator not initialized. (Call the Initialize)");

        constexpr VertexPT vertices[] =
        {
            {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)},
            {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)},
            {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)},
            {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f)},
        };

        constexpr uint32_t indices[] =
        {
            0,
            1,
            2,
            0,
            2,
            3,
        };

        outMesh.VertexFormat = eVertexFormat::PT;
        SubMesh newSubMesh = {};
        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        const int32_t pathLength = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Plane.mesh", reinterpret_cast<const char*>(VIRTUAL_ROOT_PATH));
        ASSERT(pathLength < util::MAX_PATH_LENGTH, "file path too long. length(%d), limit(%d)", pathLength, util::MAX_PATH_LENGTH);

        (void)memcpy(outMesh.MeshName, virtualFilePath, pathLength + 1);
        (void)memcpy(newSubMesh.SubMeshName, virtualFilePath, pathLength + 1);
        outMesh.MeshHash = util::GetDjb2Hash(virtualFilePath);
        newSubMesh.SubMeshHash = outMesh.MeshHash;

        const int16_t strideVertex = GetVertexStrideSize(outMesh.VertexFormat);
        const int16_t strideIndex = sBufferManager->GetIndexStrideSize();

        sBufferManager->AddVertex(reinterpret_cast<const int8_t*>(vertices), sizeof(vertices), newSubMesh.SubMeshHash, strideVertex, newSubMesh.VertexRange);
        sBufferManager->AddIndex(reinterpret_cast<const int8_t*>(indices), sizeof(indices), newSubMesh.SubMeshHash, strideIndex, newSubMesh.IndexRange);

        outMesh.VertexRange = newSubMesh.VertexRange;
        outMesh.IndexRange = newSubMesh.IndexRange;

        outMesh.SubMeshes.push_back(std::move(newSubMesh));
    }

    void MeshGenerator::CreateScreenPlane(int16_t originX, int16_t originY, int16_t width, int16_t height, Mesh& outMesh)
    {
        ASSERT(sBufferManager, "MeshGenerator not initialized. (Call the Initialize)");
        ASSERT(width > 0, "width must be > 0");
        ASSERT(height > 0, "height must be > 0");
        ASSERT(originX >= 0, "originX must be >= 0");
        ASSERT(originY >= 0, "originY must be >= 0");

        const VertexPT vertices[] =
        {
            {XMFLOAT3(static_cast<float>(originX), static_cast<float>(originY + height), 1.0f), XMFLOAT2(0.0f, 1.0f)}, // lb
            {XMFLOAT3(static_cast<float>(originX), static_cast<float>(originY), 1.0f), XMFLOAT2(0.0f, 0.0f)}, // lt
            {XMFLOAT3(static_cast<float>(originX + width), static_cast<float>(originY), 1.0f), XMFLOAT2(1.0f, 0.0f)}, // rt
            {XMFLOAT3(static_cast<float>(originX + width), static_cast<float>(originY + height), 1.0f), XMFLOAT2(1.0f, 1.0f)}, // rb
        };

        constexpr uint32_t indices[] =
        {
            0,
            1,
            2,
            0,
            2,
            3,
        };

        outMesh.VertexFormat = eVertexFormat::PT;
        SubMesh newSubMesh = {};

        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        const int32_t pathLength = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_ScreenPlane_%d_%d_%d_%d.mesh", reinterpret_cast<const char*>(VIRTUAL_ROOT_PATH), originX, originY, width, height);
        ASSERT(pathLength < util::MAX_PATH_LENGTH, "file path too long. length(%d), limit(%d)", pathLength, util::MAX_PATH_LENGTH);

        (void)memcpy(outMesh.MeshName, virtualFilePath, pathLength + 1);
        (void)memcpy(newSubMesh.SubMeshName, virtualFilePath, pathLength + 1);
        outMesh.MeshHash = util::GetDjb2Hash(virtualFilePath);
        newSubMesh.SubMeshHash = outMesh.MeshHash;

        const int16_t strideVertex = GetVertexStrideSize(outMesh.VertexFormat);
        const int16_t strideIndex = sBufferManager->GetIndexStrideSize();

        sBufferManager->AddVertex(reinterpret_cast<const int8_t*>(vertices), sizeof(vertices), newSubMesh.SubMeshHash, strideVertex, newSubMesh.VertexRange);
        sBufferManager->AddIndex(reinterpret_cast<const int8_t*>(indices), sizeof(indices), newSubMesh.SubMeshHash, strideIndex, newSubMesh.IndexRange);

        outMesh.VertexRange = newSubMesh.VertexRange;
        outMesh.IndexRange = newSubMesh.IndexRange;

        outMesh.SubMeshes.push_back(std::move(newSubMesh));
    }

    void MeshGenerator::CreateNdcPlane(Mesh& outMesh)
    {
        const VertexPT vertices[] =
        {
            {XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f)}, // lb
            {XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f)}, // lt
            {XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f)}, // rt
            {XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f)}, // rb
        };

        constexpr uint32_t indices[] =
        {
            0,
            1,
            2,
            0,
            2,
            3,
        };

        outMesh.VertexFormat = eVertexFormat::PT;
        SubMesh newSubMesh = {};

        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        const int32_t pathLength = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_NdcPlane.mesh", reinterpret_cast<const char*>(VIRTUAL_ROOT_PATH));
        ASSERT(pathLength < util::MAX_PATH_LENGTH, "file path too long. length(%d), limit(%d)", pathLength, util::MAX_PATH_LENGTH);

        (void)memcpy(outMesh.MeshName, virtualFilePath, pathLength + 1);
        (void)memcpy(newSubMesh.SubMeshName, virtualFilePath, pathLength + 1);
        outMesh.MeshHash = util::GetDjb2Hash(virtualFilePath);
        newSubMesh.SubMeshHash = outMesh.MeshHash;

        const int16_t strideVertex = GetVertexStrideSize(outMesh.VertexFormat);
        const int16_t strideIndex = sBufferManager->GetIndexStrideSize();

        sBufferManager->AddVertex(reinterpret_cast<const int8_t*>(vertices), sizeof(vertices), newSubMesh.SubMeshHash, strideVertex, newSubMesh.VertexRange);
        sBufferManager->AddIndex(reinterpret_cast<const int8_t*>(indices), sizeof(indices), newSubMesh.SubMeshHash, strideIndex, newSubMesh.IndexRange);

        outMesh.VertexRange = newSubMesh.VertexRange;
        outMesh.IndexRange = newSubMesh.IndexRange;

        outMesh.SubMeshes.push_back(std::move(newSubMesh));
    }
}
