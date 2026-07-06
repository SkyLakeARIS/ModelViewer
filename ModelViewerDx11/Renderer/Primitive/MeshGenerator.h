#pragma once
#include "../../framework.h"

namespace renderer
{
    class BufferManager;
    struct Mesh;

    class MeshGenerator
    {
    public:
        MeshGenerator() = delete;
        ~MeshGenerator() = delete;

        static void Initialize(BufferManager* const bufferManager);

        static void CreateSphere(uint16_t latLines, uint16_t lonLines, Mesh& outMesh);
        static void CreateGrid(XMFLOAT2 startPoint, uint16_t horizontalLines, uint16_t verticalLines, float gapEachLine, Mesh& outMesh);
        // TODO: improve - 각 평면(xz, yz,xy)버전을 내놓도록 하는 것이 좋을 것 같다.
        static void CreatePlane(Mesh& outMesh);
        // MEMO: origin is LeftTop of Rect(Screen)
        static void CreateScreenPlane(int16_t originX, int16_t originY, int16_t width, int16_t height, Mesh& outMesh);
    public:
        static const int8_t* const VIRTUAL_ROOT_PATH;
    private:
        static BufferManager* sBufferManager;
    };
}
