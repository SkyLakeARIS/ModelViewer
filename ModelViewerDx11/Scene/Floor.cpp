#include "Floor.h"
#include "../Util/Macro.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Resources/ModelData.h"
#include "../Renderer/Resources/BufferManager.h"
#include "../Util/Util.h"

namespace scene
{
    Floor::Floor(XMFLOAT2 startPoint, uint32_t gapEachLine, uint32_t numLineX, uint32_t numLineY)
    {
        ASSERT(numLineX >= 2, "numLineX must be 2 or greater");
        ASSERT(numLineY >= 2, "numLineY must be 2 or greater");
        ASSERT(gapEachLine >= 1, "gapEachLine must be 1 or greater");

        mNumVertices = (numLineX * 2) * (numLineY - 1) + (numLineY - 1);
        mVertices = new XMFLOAT3[mNumVertices];
        XMFLOAT3* cur = mVertices;
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

                //mVertices[lineY * numLineX + lineX] = XMFLOAT3(pos.x, 0.0f, pos.y);
                //mVertices[lineY * numLineX + lineX+1] = XMFLOAT3(pos.x, 0.0f, pos.y + gapEachLine);
            }
            // dummy
          //  mVertices[(lineY + 1) * numLineX] = mVertices[lineY * numLineX + (numLineX - 1)];
            *cur = *(cur - 1);
            ++cur;
            //mVertices[(lineY * numLineX) + numLineX] = mVertices[lineY * numLineX + (numLineX - 1)];
            pos.x = startPoint.x;
            pos.y += gapEachLine;

        }

        ID3D11Device* device = renderer::Renderer::GetInstance()->GetDevice();
        D3D11_BUFFER_DESC desc = {};


        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = sizeof(renderer::Renderer::CbWorld);
        HRESULT result = renderer::Renderer::GetInstance()->CreateConstantBuffer(desc, &mCbWorld);
        ASSERT(result == S_OK, "mCbWorld 생성 실패.");
        device->Release();

        renderer::BufferManager* const bufferManager = renderer::Renderer::GetInstance()->GetBufferManager();

        enum {MAX_FILE_PATH = 260};
        int8_t virtualFilePath[MAX_FILE_PATH] = {};
        (void)sprintf_s(reinterpret_cast<char*>(virtualFilePath), MAX_FILE_PATH, "%sPrimitive_Grid_%d_%d.mesh",
                        reinterpret_cast<const char*>(renderer::VIRTUAL_ROOT_PATH), numLineX, numLineY);

        mModelHash = util::GetDjb2Hash(virtualFilePath);
        bufferManager->AddVertexData(reinterpret_cast<int8_t*>(mVertices), sizeof(XMFLOAT3) * mNumVertices, mModelHash);
    }

    Floor::~Floor()
    {
        renderer::BufferManager* const bufferManager = renderer::Renderer::GetInstance()->GetBufferManager();
        bufferManager->RemoveVertexData(mModelHash);

        delete[] mVertices;
        SAFETY_RELEASE(mCbWorld);
    }

    void Floor::DrawNew()
    {
        renderer::Renderer::GetInstance()->SetRasterState(renderer::Renderer::eRasterType::Basic);
        renderer::Renderer::GetInstance()->SetInputLayoutTo(renderer::Renderer::eInputLayout::P);
        renderer::Renderer::GetInstance()->SetShaderTo(renderer::Renderer::eShader::Color);

        renderer::Renderer::CbWorld cbWorld;
        cbWorld.Matrix = XMMatrixIdentity();
        renderer::Renderer::GetInstance()->UpdateCbTo(mCbWorld, &cbWorld);

        renderer::Renderer::GetInstance()->BindCbToVsByObj(0, 1, &mCbWorld);
        renderer::Renderer::GetInstance()->BindCbToVsByType(1, 1, renderer::Renderer::eCbType::CbViewProj);
        renderer::Renderer::GetInstance()->BindCbToPs(0, 1, renderer::Renderer::eCbType::CbColor);


        renderer::Renderer::CbColor cbColor = {};
        cbColor.Float3 = XMFLOAT3(0.0, 1.0, 0.0);
        renderer::Renderer::GetInstance()->UpdateCB(renderer::Renderer::eCbType::CbColor, &cbColor);


        ID3D11DeviceContext* deviceContext = renderer::Renderer::GetInstance()->GetDeviceContext();
        renderer::BufferManager* const bufferManager = renderer::Renderer::GetInstance()->GetBufferManager();

        const renderer::BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);

        UINT stride = sizeof(XMFLOAT3);
        UINT offset = vertexRange.StartIndex;

        renderer::Renderer::GetInstance()->BindVertexBuffer(stride, offset);

        D3D11_PRIMITIVE_TOPOLOGY orgTopology;
        deviceContext->IAGetPrimitiveTopology(&orgTopology);

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        deviceContext->Draw(mNumVertices, 0);

        deviceContext->IASetPrimitiveTopology(orgTopology);

        deviceContext->Release();
    }
}
