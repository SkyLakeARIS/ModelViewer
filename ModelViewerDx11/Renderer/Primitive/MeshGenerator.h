#pragma once
#include "../../Core/MathPrerequisites.h"

namespace renderer
{
    struct Mesh;

    class MeshGenerator
    {
    public:
        MeshGenerator() = delete;
        ~MeshGenerator() = delete;

        static void CreateSphere(uint16_t latLines, uint16_t lonLines, Mesh& outMesh);
        static void CreateGrid(XMFLOAT2 startPoint, uint16_t horizontalLines, uint16_t verticalLines, float gapEachLine, Mesh& outMesh);
        static void CreatePlane(Mesh& outMesh);
        // MEMO: origin is LeftTop of Rect(Screen)
        static void CreateScreenPlane(int16_t originX, int16_t originY, int16_t width, int16_t height, Mesh& outMesh);
        static void CreateNdcPlane(Mesh& outMesh);
    public:
        static const int8_t* const VIRTUAL_ROOT_PATH;
    };
}
