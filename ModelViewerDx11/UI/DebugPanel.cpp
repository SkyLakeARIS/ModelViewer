#include "DebugPanel.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Primitive/MeshGenerator.h"

namespace ui
{
    DebugPanel::DebugPanel(int16_t originX, int16_t originY, int16_t width, int16_t height)
    {
        renderer::MeshGenerator::CreateScreenPlane(originX, originY, width, height, mMesh);
    }

    DebugPanel::~DebugPanel()
    {
        
    }

    void DebugPanel::Draw(renderer::Renderer& renderer)
    {
        renderer.BindInputLayoutTo(renderer::eInputLayout::PT);
        renderer.BindShaderTo(renderer::eShader::RenderToTexture);

        const int16_t strideVertex = GetVertexStrideSize(mMesh.VertexLayoutType);
        renderer.BindVertexBuffer(strideVertex);
        renderer.BindIndexBuffer();

        renderer::CbWorld cbWorldMat;
        cbWorldMat.Matrix = XMMatrixTranspose(XMMatrixIdentity());
        renderer.UpdateCB(renderer::eCbType::CbWorld, &cbWorldMat);
        renderer.BindCbToVsByType(0, 1, renderer::eCbType::CbWorld);
        renderer.BindCbToVsByType(1, 1, renderer::eCbType::CbScreenSpaceMatrix);

        renderer.BindSamplerToPsByType(0, renderer::eSamplerType::AnisotropicWrap);

        if(mType == renderer::eRenderTarget::Shadow)
        {
            renderer.BindShadowTextureToPs(0);
        }
        else
        {
            renderer.BindDefaultTextureToPs(0);
        }
        renderer.DrawIndexed(mMesh.IndexRange.Count, mMesh.IndexRange.StartIndex, mMesh.VertexRange.StartIndex);

        renderer.UnbindTexturePs(0);
    }

    void DebugPanel::SetDebugType(renderer::eRenderTarget type)
    {
        mType = type;
    }
}
