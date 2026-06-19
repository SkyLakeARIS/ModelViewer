#include "Floor.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Resources/BufferManager.h"
#include "../Renderer/Resources/ModelData.h"
#include "../Util/Define.h"
#include "../Util/Macro.h"
#include "../Util/Util.h"

namespace scene
{
    Floor::Floor(XMFLOAT2 startPoint, uint32_t gapEachLine, uint32_t numLineX, uint32_t numLineY)
    {
        ASSERT(numLineX >= 2, "numLineX must be 2 or greater");
        ASSERT(numLineY >= 2, "numLineY must be 2 or greater");
        ASSERT(gapEachLine >= 1, "gapEachLine must be 1 or greater");

        mNumVertices = (numLineX * 2) * (numLineY - 1) + (numLineY - 1);
        XMFLOAT3* vertices = new XMFLOAT3[mNumVertices];
        XMFLOAT3* cur = vertices;
        XMFLOAT2 pos(startPoint);
        for (uint32_t lineY = 0; lineY < numLineY - 1; ++lineY)
        {
            for (uint32_t lineX = 0; lineX < numLineX; ++lineX)
            {
                // triangle-strip
                *cur = XMFLOAT3(pos.x, 0.0f, pos.y);
                ++cur;
                *cur = XMFLOAT3(pos.x, 0.0f, pos.y + gapEachLine);
                ++cur;
                pos.x += gapEachLine;
            }
            *cur = *(cur - 1);
            ++cur;
            pos.x = startPoint.x;
            pos.y += gapEachLine;

        }

        renderer::BufferManager* const bufferManager = renderer::Renderer::GetInstance()->GetBufferManager();

        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        (void)sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Grid_%d_%d.mesh",
                        reinterpret_cast<const char*>(renderer::VIRTUAL_ROOT_PATH), numLineX, numLineY);

        mModelHash = util::GetDjb2Hash(virtualFilePath);
        bufferManager->AddVertexData(reinterpret_cast<int8_t*>(vertices), sizeof(XMFLOAT3) * mNumVertices, mModelHash);
    }

    Floor::~Floor()
    {
        renderer::BufferManager* const bufferManager = renderer::Renderer::GetInstance()->GetBufferManager();
        bufferManager->RemoveVertexData(mModelHash);

    }

    void Floor::Draw()
    {
        renderer::Renderer::GetInstance()->BindRasterStateByType(renderer::Renderer::eRasterType::Basic);
        renderer::Renderer::GetInstance()->BindInputLayoutTo(renderer::Renderer::eInputLayout::P);
        renderer::Renderer::GetInstance()->BindShaderTo(renderer::Renderer::eShader::Color);

        renderer::Renderer::CbWorld cbWorld;
        cbWorld.Matrix = XMMatrixIdentity();
        renderer::Renderer::GetInstance()->UpdateCB(renderer::Renderer::eCbType::CbWorld, &cbWorld);

        renderer::Renderer::GetInstance()->BindCbToVsByType(0, 1, renderer::Renderer::eCbType::CbWorld);
        renderer::Renderer::GetInstance()->BindCbToVsByType(1, 1, renderer::Renderer::eCbType::CbViewProj);
        renderer::Renderer::GetInstance()->BindCbToPs(0, 1, renderer::Renderer::eCbType::CbColor);


        renderer::Renderer::CbColor cbColor = {};
        cbColor.Float3 = XMFLOAT3(0.0, 1.0, 0.0);
        renderer::Renderer::GetInstance()->UpdateCB(renderer::Renderer::eCbType::CbColor, &cbColor);


        renderer::BufferManager* const bufferManager = renderer::Renderer::GetInstance()->GetBufferManager();

        const renderer::BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);

        UINT stride = sizeof(XMFLOAT3);
        UINT offset = vertexRange.StartIndex;

        renderer::Renderer::GetInstance()->BindVertexBuffer(stride, offset);

        D3D11_PRIMITIVE_TOPOLOGY orgTopology;
        renderer::Renderer::GetInstance()->GetCurrentPrimitiveTopology(orgTopology);

        renderer::Renderer::GetInstance()->BindPrimitiveTopologyTo(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        renderer::Renderer::GetInstance()->Draw(mNumVertices, 0);

        renderer::Renderer::GetInstance()->BindPrimitiveTopologyTo(orgTopology);

    }
}
