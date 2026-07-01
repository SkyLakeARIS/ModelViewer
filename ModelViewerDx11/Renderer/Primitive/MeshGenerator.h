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
        // TODO: improve - 좀 더 다듬는 것이 필요하다.(좀 더 써보면서 어떻게 개선하면 좋을지 고민)
        static void CreatePlane(Mesh& outMesh);
        // MEMO: origin is LeftTop of Rect(Screen)
        static void CreateScreenPlane(int16_t originX, int16_t originY, int16_t width, int16_t height, Mesh& outMesh);
    public:
        static const int8_t* const VIRTUAL_ROOT_PATH;
    private:
        static BufferManager* sBufferManager;
    };
}
