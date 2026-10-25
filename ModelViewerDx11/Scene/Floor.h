#pragma once
#include "../Renderer/Resources/Mesh.h"

namespace scene
{
    class Floor
    {
    public:
        Floor(XMFLOAT2 startPoint, uint32_t gapEachLine, uint32_t numLineX, uint32_t numLineY);
        ~Floor();

        void SubmitCommand();
    private:
        renderer::Mesh mMesh;
    };
}
