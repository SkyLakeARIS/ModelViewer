#pragma once
#include <vector>
#include "../../Core/MathPrerequisites.h"

namespace renderer
{
    struct VertexPTN;
    struct VertexPT;
    struct VertexP;
    struct Mesh;

    class MeshGenerator
    {
    public:
        MeshGenerator() = delete;
        ~MeshGenerator() = delete;

        static void CreateSphere(uint16_t latLines, uint16_t lonLines, Mesh& outMesh, std::vector<VertexP>& outVertices, std::vector<uint32_t>& outIndices);
        static void CreateGrid(XMFLOAT2 startPoint, uint16_t horizontalLines, uint16_t verticalLines, float gapEachLine, Mesh& outMesh, std::vector<VertexPTN>& outVertices, std::vector<uint32_t>& outIndices);
        static void CreatePlane(Mesh& outMesh, std::vector<VertexPT>& outVertices, std::vector<uint32_t>& outIndices);
        // MEMO: origin is LeftTop of Rect(Screen)
        static void CreateScreenPlane(int16_t originX, int16_t originY, int16_t width, int16_t height, Mesh& outMesh);
        static void CreateNdcPlane(Mesh& outMesh);
    public:
        static const int8_t* const VIRTUAL_ROOT_PATH;
    };
}
