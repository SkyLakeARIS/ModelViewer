#include "MeshGenerator.h"
#include "../../Util/Macro.h"
#include "../../Util/Util.h"
#include "../Resources/BufferManager.h"
#include "../Resources/ModelData.h"
#include "../Resources/RenderTypes.h"

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

        const uint32 numVertex = ((latLines - 2) * lonLines) + 2;
        const uint32 numFace = ((latLines - 3) * (lonLines) * 2) + (lonLines * 2);

        std::vector<VertexP> vertices(numVertex);

        vertices[0].Position.x = 0.0f;
        vertices[0].Position.y = 0.0f;
        vertices[0].Position.z = 1.0f;

        XMMATRIX matRotationX;
        XMMATRIX matRotationY;
        float sphereYaw = 0.0f;
        float spherePitch = 0.0f;
        XMVECTOR currVertPos = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        for (uint32 i = 0; i < latLines - 2U; ++i)
        {
            spherePitch = (float)(i + 1U) * (3.14f / (float)(latLines - 1U));
            matRotationX = XMMatrixRotationX(spherePitch);
            for (uint32 j = 0U; j < lonLines; ++j)
            {
                sphereYaw = (float)j * (6.28f / (float)lonLines);
                matRotationY = XMMatrixRotationZ(sphereYaw);
                currVertPos = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (matRotationX * matRotationY));
                currVertPos = XMVector3Normalize(currVertPos);
                uint32 index = i * lonLines + j + 1U;
                vertices[index].Position.x = XMVectorGetX(currVertPos);
                vertices[index].Position.y = XMVectorGetY(currVertPos);
                vertices[index].Position.z = XMVectorGetZ(currVertPos);
            }
        }

        vertices[numVertex - 1U].Position.x = 0.0f;
        vertices[numVertex - 1U].Position.y = 0.0f;
        vertices[numVertex - 1U].Position.z = -1.0f;

        std::vector<uint32> indices(numFace * 3U);

        int k = 0;
        for (uint32 l = 0U; l < lonLines - 1U; ++l)
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

        for (uint32 i = 0U; i < latLines - 3U; ++i)
        {
            for (uint32 j = 0U; j < lonLines - 1U; ++j)
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

        for (uint32 l = 0U; l < lonLines - 1U; ++l)
        {
            indices[k] = (numVertex - 1U) - (l + 2U);
            indices[k + 1] = (numVertex - 1U) - (l + 1U);
            indices[k + 2] = numVertex - 1U;
            k += 3;
        }

        indices[k] = numVertex - 2U;
        indices[k + 1] = (numVertex - 1U) - lonLines;
        indices[k + 2] = numVertex - 1U;


        outMesh.VertexLayoutType = eInputLayout::P;

        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        const int32_t pathLength = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Sphere_%d_%d.mesh", reinterpret_cast<const char*>(VIRTUAL_ROOT_PATH), latLines, lonLines);
        ASSERT(pathLength < util::MAX_PATH_LENGTH, "file path too long. length(%d), limit(%d)", pathLength, util::MAX_PATH_LENGTH);

        (void)memcpy(outMesh.MeshName, virtualFilePath, pathLength + 1);
        outMesh.MeshHash = util::GetDjb2Hash(virtualFilePath);

        const int16_t strideVertex = GetVertexStrideSize(outMesh.VertexLayoutType);
        const int16_t strideIndex = sBufferManager->GetIndexStrideSize();

        sBufferManager->AddVertex(reinterpret_cast<int8_t*>(vertices.data()), strideVertex * vertices.size(), outMesh.MeshHash, strideVertex, outMesh.VertexRange);
        sBufferManager->AddIndex(reinterpret_cast<int8_t*>(indices.data()), strideIndex * indices.size(), outMesh.MeshHash, strideIndex, outMesh.IndexRange);
    }

    void MeshGenerator::CreateGrid(XMFLOAT2 startPoint, uint16_t horizontalLines, uint16_t verticalLines, float gapEachLine, Mesh& outMesh)
    {
        ASSERT(sBufferManager, "MeshGenerator not initialized. (Call the Initialize)");
        ASSERT(horizontalLines >= 2, "horizontalLines must be 2 or greater. passed(%d)", horizontalLines);
        ASSERT(verticalLines >= 2, "verticalLines must be 2 or greater. passed(%d)", verticalLines);
        ASSERT(gapEachLine >= 1, "gapEachLine must be 1 or greater");

        const uint32_t numRows = horizontalLines - 1;
        const int32_t numVertices = (verticalLines * 2) * numRows + (numRows - 1) * 2;
        std::unique_ptr<XMFLOAT3[]> vertices = std::make_unique<XMFLOAT3[]>(numVertices);

        XMFLOAT3* cur = vertices.get();
        XMFLOAT2  pos(startPoint);
        for (uint32_t lineY = 0; lineY < horizontalLines - 1; ++lineY)
        {
            for (uint32_t lineX = 0; lineX < verticalLines; ++lineX)
            {
                // triangle-strip
                *cur = XMFLOAT3(pos.x, 0.0f, pos.y);
                ++cur;
                *cur = XMFLOAT3(pos.x, 0.0f, pos.y + gapEachLine);
                ++cur;
                pos.x += gapEachLine;
            }
            if (lineY < numRows - 1)
            {
                *cur = *(cur - 1);
                ++cur;
                pos.x = startPoint.x;
                pos.y += gapEachLine;
                *cur = XMFLOAT3(pos.x, 0.0f, pos.y);
                ++cur;
            }
        }

        outMesh.VertexLayoutType = eInputLayout::P;

        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        const int32_t pathLength = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Grid_%d_%d.mesh", reinterpret_cast<const char*>(VIRTUAL_ROOT_PATH), verticalLines, horizontalLines);
        ASSERT(pathLength < util::MAX_PATH_LENGTH, "file path too long. length(%d), limit(%d)", pathLength, util::MAX_PATH_LENGTH);

        (void)memcpy(outMesh.MeshName, virtualFilePath, pathLength + 1);
        outMesh.MeshHash = util::GetDjb2Hash(virtualFilePath);

        const int16_t strideVertex = GetVertexStrideSize(outMesh.VertexLayoutType);

        sBufferManager->AddVertex(reinterpret_cast<int8_t*>(vertices.get()), strideVertex * numVertices, outMesh.MeshHash, strideVertex, outMesh.VertexRange);
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

        outMesh.VertexLayoutType = eInputLayout::PT;

        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        const int32_t pathLength = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Plane.mesh", reinterpret_cast<const char*>(VIRTUAL_ROOT_PATH));
        ASSERT(pathLength < util::MAX_PATH_LENGTH, "file path too long. length(%d), limit(%d)", pathLength, util::MAX_PATH_LENGTH);

        (void)memcpy(outMesh.MeshName, virtualFilePath, pathLength + 1);
        outMesh.MeshHash = util::GetDjb2Hash(virtualFilePath);

        const int16_t strideVertex = GetVertexStrideSize(outMesh.VertexLayoutType);
        const int16_t strideIndex = sBufferManager->GetIndexStrideSize();

        sBufferManager->AddVertex(reinterpret_cast<const int8_t*>(vertices), sizeof(vertices), outMesh.MeshHash, strideVertex, outMesh.VertexRange);
        sBufferManager->AddIndex(reinterpret_cast<const int8_t*>(indices), sizeof(indices), outMesh.MeshHash, strideIndex, outMesh.IndexRange);

    }
}
