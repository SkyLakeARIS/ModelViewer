#pragma once
#include "../framework.h"

namespace scene
{
    class Floor
    {

    public:
        Floor(XMFLOAT2 startPoint, uint32_t gapEachLine, uint32_t numLineX, uint32_t numLineY);
        ~Floor();

        void Draw();
    private:

        XMFLOAT3* mVertices;
        uint32_t mNumVertices;

        ID3D11Buffer* mVerticesBuffer;
        ID3D11Buffer* mCbWorld;
    };
}