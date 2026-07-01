#pragma once
#include "../Renderer/Resources/ModelData.h"

namespace renderer
{
    class Renderer;
}

namespace ui
{
    class DebugPanel
    {
    public:
        DebugPanel(int16_t originX, int16_t originY, int16_t width, int16_t height);
        ~DebugPanel();

        void Draw(renderer::Renderer& renderer);

        void SetDebugType(renderer::eRenderTarget type);
    private:
        renderer::Mesh mMesh;
        renderer::eRenderTarget mType;
    };
}
