#include "Floor.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Resources/BufferManager.h"
#include "../Renderer/Resources/ModelData.h"
#include "../Util/Define.h"
#include "../Util/Macro.h"
#include "../Util/Util.h"

namespace scene
{
    Floor::Floor(XMFLOAT2 startPoint, uint32_t gapEachLine, uint32_t numLineX, uint32_t numLineY, renderer::BufferManager* const bufferManager, renderer::Renderer& renderer)
        : mBufferManager(bufferManager)
    {
        ASSERT(numLineX >= 2, "numLineX must be 2 or greater");
        ASSERT(numLineY >= 2, "numLineY must be 2 or greater");
        ASSERT(gapEachLine >= 1, "gapEachLine must be 1 or greater");

        mNumVertices       = (numLineX * 2) * (numLineY - 1) + (numLineY - 1);
        XMFLOAT3* vertices = new XMFLOAT3[mNumVertices];
        XMFLOAT3* cur      = vertices;
        XMFLOAT2  pos(startPoint);
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


        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        const int16_t wroteCount = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Grid_%d_%d.mesh",
                        reinterpret_cast<const char*>(renderer::VIRTUAL_ROOT_PATH), numLineX, numLineY);

        mModelHash = util::GetDjb2Hash(virtualFilePath);
        mMesh.MeshHash = util::GetDjb2Hash(virtualFilePath);
        (void)memcpy(mMesh.MeshName, virtualFilePath, wroteCount+1);
        mMesh.VertexLayoutType = renderer::eInputLayout::P;
        const int16_t stride = renderer::GetVertexStrideSize(mMesh.VertexLayoutType);
        bufferManager->AddVertexData(reinterpret_cast<int8_t*>(vertices), stride * mNumVertices, mMesh.MeshHash, stride, mMesh.VertexRange);
        delete[] vertices;
    }

    Floor::~Floor()
    {
        const int16_t stride = renderer::GetVertexStrideSize(mMesh.VertexLayoutType);
        mBufferManager->RemoveVertexData(stride, mMesh.MeshHash);
        mBufferManager = nullptr;

        // TODO: BufferManager를 받지 않고, BufferData 처리할 수 있는 로직이 필요함.
    }

    void Floor::Draw(renderer::Renderer& renderer)
    {
        renderer.BindRasterStateByType(renderer::eRasterType::Basic);
        renderer.BindInputLayoutTo(renderer::eInputLayout::P);
        renderer.BindShaderTo(renderer::eShader::Color);

        renderer::CbWorld cbWorld;
        cbWorld.Matrix = XMMatrixIdentity();
        renderer.UpdateCB(renderer::eCbType::CbWorld, &cbWorld);

        renderer.BindCbToVsByType(0, 1, renderer::eCbType::CbWorld);
        renderer.BindCbToVsByType(1, 1, renderer::eCbType::CbViewProj);
        renderer.BindCbToPs(0, 1, renderer::eCbType::CbColor);


        renderer::CbColor cbColor = {};
        cbColor.Float3 = XMFLOAT3(0.0, 1.0, 0.0);
        renderer.UpdateCB(renderer::eCbType::CbColor, &cbColor);





        const int16_t stride = renderer::GetVertexStrideSize(mMesh.VertexLayoutType);
        renderer.BindVertexBufferNew(stride, 0);

        D3D11_PRIMITIVE_TOPOLOGY orgTopology;
        renderer.GetCurrentPrimitiveTopology(orgTopology);

        renderer.BindPrimitiveTopologyTo(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        renderer.Draw(mMesh.VertexRange.Count, mMesh.VertexRange.StartIndex);

        renderer.BindPrimitiveTopologyTo(orgTopology);

    }
}
