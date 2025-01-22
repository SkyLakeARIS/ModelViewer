#include "Floor.h"

#include "Renderer.h"

Floor::Floor(XMFLOAT2 startPoint, uint32_t gapEachLine, uint32_t numLineX, uint32_t numLineY)
{
    assert(numLineX >= 2, "numLineX must be 2 or greater");
    assert(numLineY >= 2, "numLineY must be 2 or greater");
    assert(gapEachLine >= 1, "gapEachLine must be 1 or greater");

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

    ID3D11Device* device = Renderer::GetInstance()->GetDevice();
    D3D11_BUFFER_DESC desc={};
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.ByteWidth = sizeof(XMFLOAT3) * mNumVertices;
    desc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA subResource={};
    subResource.pSysMem = mVertices;

    HRESULT result = device->CreateBuffer(&desc, &subResource, &mVerticesBuffer);
    assert(result == S_OK, "fail to create Buffer ");


    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = sizeof(Renderer::CbWorld);
    result = Renderer::GetInstance()->CreateConstantBuffer(desc, &mCbWorld);
    ASSERT(result == S_OK, "mCbWorld 생성 실패.");
    device->Release();
}

Floor::~Floor()
{
    delete[] mVertices;
    SAFETY_RELEASE(mCbWorld);
    SAFETY_RELEASE(mVerticesBuffer);
}

void Floor::Draw()
{
    Renderer::GetInstance()->SetRasterState(Renderer::eRasterType::Basic);
    Renderer::GetInstance()->SetInputLayoutTo(Renderer::eInputLayout::P);
    Renderer::GetInstance()->SetShaderTo(Renderer::eShader::Color);

    Renderer::CbWorld cbWorld;
    cbWorld.Matrix = XMMatrixIdentity();
    Renderer::GetInstance()->UpdateCbTo(mCbWorld, &cbWorld);

    Renderer::GetInstance()->BindCbToVsByObj(0, 1, &mCbWorld);
    Renderer::GetInstance()->BindCbToVsByType(1, 1, Renderer::eCbType::CbViewProj);
    Renderer::GetInstance()->BindCbToPs(0, 1, Renderer::eCbType::CbColor);




    Renderer::CbColor cbColor={};
    cbColor.Float3 = XMFLOAT3(0.0, 1.0, 0.0);
    Renderer::GetInstance()->UpdateCB(Renderer::eCbType::CbColor, &cbColor);

    ID3D11DeviceContext* deviceContext = Renderer::GetInstance()->GetDeviceContext();
    UINT stride = sizeof(XMFLOAT3);
    UINT offset = 0;
    deviceContext->IASetVertexBuffers(0, 1, &mVerticesBuffer, &stride, &offset);
    D3D11_PRIMITIVE_TOPOLOGY orgTopology;
    deviceContext->IAGetPrimitiveTopology(&orgTopology);

    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    deviceContext->Draw(mNumVertices, 0);

    deviceContext->IASetPrimitiveTopology(orgTopology);

    deviceContext->Release();
}
