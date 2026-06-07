#include "Light.h"
#include "Camera.h"
#include "../Util/Macro.h"
#include "../Renderer/Primitive/Plane.h"
#include "../Renderer/Renderer.h"

namespace scene
{
    Light::Light(XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 color, Camera* camera, float nearPlane, float farPlane)
        : mPosition(pos)
        , mDirection(dir)
        , mColor(color)
        , mMatProj(XMMatrixIdentity())
        , mMatViewProj(XMMatrixIdentity())
        , mMatWorld(XMMatrixIdentity())
        , mNearPlane(nearPlane)
        , mFarPlane(farPlane)
        , mCamera(camera)
        //, mLinesBuffer(nullptr)
    {
        mMesh = new renderer::Plane();
        mMesh->SetPosition(mPosition);


        mLines.reserve(24 * eCascadeLevel::Level_4);
        mCascadePlaneDistances[0] = nearPlane; // 0.1
        mCascadePlaneDistances[1] = farPlane / 100.0f;   // 5
        mCascadePlaneDistances[2] = farPlane / 50.0f; // 10 /
        mCascadePlaneDistances[3] = farPlane / 25.0f; // 20
        mCascadePlaneDistances[4] = farPlane / 10.0f; // 50
        mCascadePlaneDistances[5] = farPlane; // 500

        updateLightPropertyCB();
        updateMatrices();
    }

    Light::~Light()
    {
        mMesh->UnbindTexture();
        if (mMesh)
        {
            delete mMesh;
            mMesh = nullptr;
        }

        //SAFETY_RELEASE(mLinesBuffer);


    }

    void Light::Initialize()
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        //   desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 1;
        desc.Texture2D.MostDetailedMip = 0;
        ID3D11ShaderResourceView* texLightIcon = nullptr;

        HRESULT result = renderer::Renderer::GetInstance()->CreateTextureResource(L"./AssetData/textures/lightIcon.png", WIC_FLAGS_NONE, desc, &texLightIcon);
        ASSERT(SUCCEEDED(result), "Light Init - failed to ready texture");
        SET_PRIVATE_DATA(texLightIcon, "TexLightIcon");

        mMesh->SetTexture(texLightIcon);
        texLightIcon->AddRef();

        ID3D11Device* device = renderer::Renderer::GetInstance()->GetDevice();

        // alpha blend 
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = true;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        // TODO: 좀 더 조사가 필요한 부분: 이런 정해지지 않은 것들은 어떻게 깔끔하게 처리할 수 있을지? 동적 생성은 피할 수 없는 부분인지?
        renderer::Renderer::GetInstance()->CreateBlendState(blendDesc, mBlendHash);


        device->Release();
        SAFETY_RELEASE(texLightIcon);
    }

    void Light::Update(Camera* camera)
    {
        mMesh->Update();
        mMesh->GetWorldMatrix(mMatWorld);


        renderer::Renderer::CbWorld cbMatWorld;
        cbMatWorld.Matrix = XMMatrixTranspose(mMatWorld);
        renderer::Renderer::GetInstance()->UpdateCB(renderer::Renderer::eCbType::CbWorld, &cbMatWorld);
    }

    void Light::Draw()
    {
        // MEMO: 임시로 CB 업데이트하도록 강제로 넣음. (cascade 테스트) update, rendering 주기가 달라서.
        Update(nullptr);

        renderer::Renderer::GetInstance()->SetInputLayoutTo(renderer::Renderer::eInputLayout::PT);
        renderer::Renderer::GetInstance()->SetShaderTo(renderer::Renderer::eShader::RenderToTexture);

        ID3D11DeviceContext* deviceContext = renderer::Renderer::GetInstance()->GetDeviceContext();
        renderer::Renderer::GetInstance()->SetRasterState(renderer::Renderer::eRasterType::Basic);
        renderer::Renderer::GetInstance()->BindBlendStateByHash(mBlendHash, nullptr, 0xffffffff);

        deviceContext->Release();


        renderer::Renderer::GetInstance()->BindCbToVsByType(0U, 1U, renderer::Renderer::eCbType::CbWorld);
        renderer::Renderer::GetInstance()->BindCbToVsByType(1U, 1U, renderer::Renderer::eCbType::CbViewProj);

        //mMesh->Draw();
        mMesh->Draw();
    }

    //void Light::DrawDebug()
    //{
    //    renderer::Renderer::GetInstance()->SetInputLayoutTo(renderer::Renderer::eInputLayout::P);
    //    renderer::Renderer::GetInstance()->SetShaderTo(renderer::Renderer::eShader::Color);

    //    renderer::Renderer::CbColor cbColor = {  };
    //    cbColor.Float3 = XMFLOAT3(1.0f, 1.0f, 0.0f);

    //    renderer::Renderer::GetInstance()->UpdateCB(renderer::Renderer::eCbType::CbColor, &cbColor);
    //    renderer::Renderer::GetInstance()->BindCbToPs(0, 1, renderer::Renderer::eCbType::CbColor);

    //    renderer::Renderer::CbWorld cbWorld;
    //    cbWorld.Matrix = XMMatrixTranspose(XMMatrixIdentity());
    //    renderer::Renderer::GetInstance()->UpdateCB(renderer::Renderer::eCbType::CbWorld, &cbWorld);

    //    renderer::Renderer::GetInstance()->BindCbToVsByType(0U, 1U, renderer::Renderer::eCbType::CbWorld);
    //    renderer::Renderer::GetInstance()->BindCbToVsByType(1, 1, renderer::Renderer::eCbType::CbViewProj);

    //    ID3D11DeviceContext* deviceContext = renderer::Renderer::GetInstance()->GetDeviceContext();
    //    const UINT stride = sizeof(XMFLOAT3);
    //    const UINT offset = 0;
    //    deviceContext->IASetVertexBuffers(0, 1, &mLinesBuffer, &stride, &offset);

    //    D3D11_PRIMITIVE_TOPOLOGY origTopology;
    //    deviceContext->IAGetPrimitiveTopology(&origTopology);
    //    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    //    deviceContext->Draw(mLines.size(), 0);

    //    deviceContext->IASetPrimitiveTopology(origTopology);

    //    deviceContext->Release();
    //}

    void Light::Move(double deltaTime, float direction)
    {
        XMFLOAT3 dir = mDirection;
        XMFLOAT3 pos = mPosition;
        XMVECTOR vDir = XMLoadFloat3(&dir);
        XMVECTOR vPos = XMLoadFloat3(&pos);
        vDir *= deltaTime * 100.0;
        vPos += vDir;
        XMStoreFloat3(&mPosition, (vPos));
        mMesh->SetPosition(mPosition);

        updateMatrices();
        updateLightPropertyCB();
    }

    void Light::SetupCascade()
    {
        mLines.clear();
        for (uint32_t i = 0; i < eCascadeLevel::Level_4 - 1; ++i)
        {
            getPointsFromMatrix(&(mCamera->GetViewMatrix()), mCascadePlaneDistances[i], mCascadePlaneDistances[i + 1], &mMatLightViews[i], &mMatLightProjs[i]);
        }
        // int32_t index = 0;
       //  mMatViewProj = mMatLightViews[index] * mMatLightProjs[index];

        // TODO: 이건 새로운 유형의 버퍼(Dynamic)를 만들어야 하지 않을까?
        //if (!mLinesBuffer)
        //{
        //    // debug - frustum
        //    D3D11_BUFFER_DESC bufferDesc = {  };
        //    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        //    bufferDesc.ByteWidth = sizeof(XMFLOAT3) * mLines.size();
        //    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        //    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        //    D3D11_SUBRESOURCE_DATA data;
        //    data.pSysMem = mLines.data();

        //    ID3D11Device* device = renderer::Renderer::GetInstance()->GetDevice();
        //    device->CreateBuffer(&bufferDesc, &data, &mLinesBuffer);
        //    device->Release();
        //}
        //else
        {
            //ID3D11DeviceContext* deviceContext = renderer::Renderer::GetInstance()->GetDeviceContext();
            //D3D11_MAPPED_SUBRESOURCE subresource = {  };
            //subresource.pData = mLines.data();
            //subresource.RowPitch = sizeof(XMFLOAT3) * mLines.size();
            //deviceContext->Map(mLinesBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
            ////  D3D11_MAP_FLAG::D3D11_MAP_FLAG_DO_NOT_WAIT

            //deviceContext->Unmap(mLinesBuffer, 0);
            //deviceContext->Release();
        }

        updateMatrices();
    }

    void Light::SetDirection(XMFLOAT3 dir)
    {
        XMFLOAT3 pos = mPosition;
        XMVECTOR vDir = XMLoadFloat3(&dir);
        XMVECTOR vPos = XMLoadFloat3(&pos);
        vDir = vDir - vPos;
        XMStoreFloat3(&mDirection, (vDir));

        // mDirection = dir;

        updateMatrices();
    }

    void Light::SetColor(XMFLOAT3 color)
    {
        mColor = color;

        updateLightPropertyCB();
    }

    XMFLOAT4 Light::GetDirection() const
    {
        return XMFLOAT4(mDirection.x, mDirection.y, mDirection.z, 1.0f);
    }

    XMFLOAT4 Light::GetPosition() const
    {
        return XMFLOAT4(mPosition.x, mPosition.y, mPosition.z, 1.0f);
    }

    XMFLOAT4 Light::GetColor() const
    {
        return XMFLOAT4(mColor.x, mColor.y, mColor.z, 1.0f);
    }

    const XMMATRIX* const Light::GetViewProjMatrix() const
    {
        return &mMatViewProj;
    }

    void Light::updateMatrices()
    {
        mMatView = XMMatrixLookAtLH(XMLoadFloat3(&mPosition), XMLoadFloat3(&mDirection), XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));
        //mMatProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.0, 69.0f, 120.0f);
        mMatProj = XMMatrixOrthographicOffCenterLH(-1.0f, 1.0f, -1.0f, 1.0f, 69.0f, 100.0f);
        mMatViewProj = mMatView * mMatProj;


        // cascade test
        int32_t index = 0;
        mMatViewProj = mMatLightViews[index] * mMatLightProjs[index];


        renderer::Renderer::CbLightViewProjMatrix cbLightVpMat;
        cbLightVpMat.Matrix = XMMatrixTranspose(mMatViewProj);
        //deviceContext->UpdateSubresource(mCB, 0, nullptr, &cbWvp, 0U, 0U);
        renderer::Renderer::GetInstance()->UpdateCB(renderer::Renderer::eCbType::CbLightViewProjMatrix, &cbLightVpMat);

    }

    void Light::updateLightPropertyCB()
    {
        renderer::Renderer::CbLightProperty cbLightProperty;
        cbLightProperty.First = XMFLOAT4(mColor.x, mColor.y, mColor.z, 0.0f);
        cbLightProperty.Second = XMFLOAT4(mPosition.x, mPosition.y, mPosition.z, 0.0f);
        renderer::Renderer::GetInstance()->UpdateCB(renderer::Renderer::eCbType::CbLightProperty, &cbLightProperty);
    }

    void Light::getPointsFromMatrix(XMMATRIX* matView, float nearPlane, float farPlane, XMMATRIX* const outMatLightView, XMMATRIX* const outMatLightProj)
    {
        XMMATRIX matLightProj = XMMatrixPerspectiveFovLH(mCamera->GetFov(), mCamera->GetAspectRatio(), nearPlane, farPlane);
        matLightProj = mCamera->GetViewMatrix() * matLightProj;
        XMMATRIX matViewProjInv = XMMatrixInverse(nullptr, matLightProj);

        // near - leftTop, rightTop, leftBottom, rightBottom
        // far - leftTop, rightTop, leftBottom, rightBottom
        XMFLOAT3 points[8] = {
            //{-1.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {-1.0, -1.0, 0.0}, {1.0, -1.0, 0.0},
            //{-1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {-1.0, -1.0, 1.0}, {1.0, -1.0, 1.0}
            {-1.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {-1.0, -1.0, 0.0}, {1.0, -1.0, 0.0},
            {-1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {-1.0, -1.0, 1.0}, {1.0, -1.0, 1.0}
        };

        XMFLOAT3 pointsInWorld[8] = {};
        XMVECTOR mid = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

        for (uint32_t i = 0; i < 8; ++i)
        {
            XMVECTOR point = XMLoadFloat3(&points[i]);
            point = XMVector3Transform(point, matViewProjInv);
            point /= point.m128_f32[3];
            XMStoreFloat3(&pointsInWorld[i], point);
            mid += point;
        }
        mid /= 8;

        XMVECTOR radius = XMVector3Length((XMLoadFloat3(&pointsInWorld[0]) - XMLoadFloat3(&pointsInWorld[6])));
        radius *= 0.5f;


        float texelPerUnit = 2048.0f / (radius.m128_f32[0] * 2.0f); // 2048 is shadow texture resolution
        const XMMATRIX matScale = XMMatrixScaling(texelPerUnit, texelPerUnit, texelPerUnit);

        XMVECTOR vecLightDir = XMVector3Normalize(XMLoadFloat3(&mDirection));
        XMMATRIX tempMatView = XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), -vecLightDir, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        tempMatView = tempMatView * matScale;
        const XMMATRIX tempMatViewInv = XMMatrixInverse(nullptr, tempMatView);

        // 텍셀 사이즈에서(?) 프러스텀 중앙의 위치 이동
        mid = XMVector3Transform(mid, tempMatView);
        mid.m128_f32[0] = floorf(mid.m128_f32[0]);
        mid.m128_f32[1] = floorf(mid.m128_f32[1]);
        mid = XMVector3Transform(mid, tempMatViewInv);

        XMVECTOR eyePosition = mid + (vecLightDir * radius * 2.0f);

        *outMatLightView = XMMatrixLookAtLH(eyePosition, mid, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        // *outMatLightView = XMMatrixLookToLH(eyePosition, -vecLightDir, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

        float radiusF = radius.m128_f32[0];
        *outMatLightProj = XMMatrixOrthographicOffCenterLH(-radiusF, radiusF, -radiusF, radiusF, -radiusF * 6.0f, radiusF * 6.0f);


        XMFLOAT3 asdasd;
        XMStoreFloat3(&asdasd, eyePosition);
        mLines.push_back(asdasd);
        XMStoreFloat3(&asdasd, mid);
        mLines.push_back(asdasd);

        // draw frustum
        //mLines.push_back(XMFLOAT3(pointsFinal[0])); //  near
        //mLines.push_back(XMFLOAT3(pointsFinal[1]));

        //mLines.push_back(XMFLOAT3(pointsFinal[1]));
        //mLines.push_back(XMFLOAT3(pointsFinal[3]));

        //mLines.push_back(XMFLOAT3(pointsFinal[3]));
        //mLines.push_back(XMFLOAT3(pointsFinal[2]));

        //mLines.push_back(XMFLOAT3(pointsFinal[2]));
        //mLines.push_back(XMFLOAT3(pointsFinal[0]));

        //mLines.push_back(XMFLOAT3(pointsFinal[0])); // near to far
        //mLines.push_back(XMFLOAT3(pointsFinal[4]));

        //mLines.push_back(XMFLOAT3(pointsFinal[1]));
        //mLines.push_back(XMFLOAT3(pointsFinal[5]));

        //mLines.push_back(XMFLOAT3(pointsFinal[2]));
        //mLines.push_back(XMFLOAT3(pointsFinal[6]));

        //mLines.push_back(XMFLOAT3(pointsFinal[3]));
        //mLines.push_back(XMFLOAT3(pointsFinal[7]));

        //mLines.push_back(XMFLOAT3(pointsFinal[4])); // far
        //mLines.push_back(XMFLOAT3(pointsFinal[5]));

        //mLines.push_back(XMFLOAT3(pointsFinal[5]));
        //mLines.push_back(XMFLOAT3(pointsFinal[7]));

        //mLines.push_back(XMFLOAT3(pointsFinal[7]));
        //mLines.push_back(XMFLOAT3(pointsFinal[6]));

        //mLines.push_back(XMFLOAT3(pointsFinal[6]));
        //mLines.push_back(XMFLOAT3(pointsFinal[4]));


        XMFLOAT3 pointToWorld[8] = {
        };

        XMMATRIX lightViewProjInv = XMMatrixInverse(nullptr, (*outMatLightView) * (*outMatLightProj));
        XMVECTOR vecPointToWorld;
        // move points light view space to world space
        for (uint32_t i = 0; i < 8; ++i)
        {
            vecPointToWorld = XMLoadFloat3(&points[i]);
            vecPointToWorld = XMVector3Transform(vecPointToWorld, lightViewProjInv);
            vecPointToWorld /= vecPointToWorld.m128_f32[3];
            XMStoreFloat3(&pointToWorld[i], vecPointToWorld);
        }

        mLines.push_back(XMFLOAT3(pointToWorld[0])); //  near
        mLines.push_back(XMFLOAT3(pointToWorld[1]));

        mLines.push_back(XMFLOAT3(pointToWorld[1]));
        mLines.push_back(XMFLOAT3(pointToWorld[3]));

        mLines.push_back(XMFLOAT3(pointToWorld[3]));
        mLines.push_back(XMFLOAT3(pointToWorld[2]));

        mLines.push_back(XMFLOAT3(pointToWorld[2]));
        mLines.push_back(XMFLOAT3(pointToWorld[0]));

        mLines.push_back(XMFLOAT3(pointToWorld[0])); // near to far
        mLines.push_back(XMFLOAT3(pointToWorld[4]));

        mLines.push_back(XMFLOAT3(pointToWorld[1]));
        mLines.push_back(XMFLOAT3(pointToWorld[5]));

        mLines.push_back(XMFLOAT3(pointToWorld[2]));
        mLines.push_back(XMFLOAT3(pointToWorld[6]));

        mLines.push_back(XMFLOAT3(pointToWorld[3]));
        mLines.push_back(XMFLOAT3(pointToWorld[7]));

        mLines.push_back(XMFLOAT3(pointToWorld[4])); // far
        mLines.push_back(XMFLOAT3(pointToWorld[5]));

        mLines.push_back(XMFLOAT3(pointToWorld[5]));
        mLines.push_back(XMFLOAT3(pointToWorld[7]));

        mLines.push_back(XMFLOAT3(pointToWorld[7]));
        mLines.push_back(XMFLOAT3(pointToWorld[6]));

        mLines.push_back(XMFLOAT3(pointToWorld[6]));
        mLines.push_back(XMFLOAT3(pointToWorld[4]));

    }
}
