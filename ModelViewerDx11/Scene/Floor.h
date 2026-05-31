#pragma once
#include "../framework.h"

namespace scene
{
    // TODO: 클래스 말고 Generator에서 Grid Mesh를 생성하고 floor 객체(Plane 타입)로 관리되는 게 좋아 보임.
    class Floor
    {

    public:
        Floor(XMFLOAT2 startPoint, uint32_t gapEachLine, uint32_t numLineX, uint32_t numLineY);
        ~Floor();

        void Draw();
        void DrawNew();
    private:

        XMFLOAT3* mVertices;
        uint32_t mNumVertices;

        HashID mModelHash;

        ID3D11Buffer* mVerticesBuffer;
        ID3D11Buffer* mCbWorld;
    };
}