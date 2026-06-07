#include "Sky.h"
#include "Camera.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Resources/BufferManager.h"
#include "../Util/Macro.h"
#include "../Util/Util.h"

namespace scene
{
    Sky::Sky(Camera& camera)
        : mCamera(&camera)
        , mModelHash(0)
        , mWorld(XMMatrixIdentity())
        , mLatLines(0)
        , mLonLines(0)
    {

        // TODO: 이런 상수 값들도 따로 모아놓을 파일을 만드는 게 좋아 보임
        enum
        {
            MAX_FILE_PATH = 260
        };
        int8_t virtualFilePath[MAX_FILE_PATH] = {};
        (void)sprintf_s(reinterpret_cast<char*>(virtualFilePath), MAX_FILE_PATH, "%sPrimitive_Sphere_%d_%d.mesh",
                  reinterpret_cast<const char*>(renderer::VIRTUAL_ROOT_PATH), mLonLines, mLatLines);

        mModelHash = util::GetDjb2Hash(virtualFilePath);
    }

    Sky::~Sky()
    {


        mCamera = nullptr;





        SAFETY_RELEASE(mMesh.Texture);

    }

    HRESULT Sky::Initialize(uint32 latLines, uint32 lonLines)
    {

        // init mesh info
        HRESULT result;
        result = createSphere(latLines, lonLines);
        if(FAILED(result))
        {
            return result;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = 1;
        srvDesc.TextureCube.MostDetailedMip = 0;
        
        result = renderer::Renderer::GetInstance()->CreateDdsTextureResource(L"./AssetData/textures/skybox.dds", DDS_FLAGS_NONE, srvDesc, &mMesh.Texture);
        if (FAILED(result))
        {
            ASSERT(false, "Skybox - fail to create texture");
            return result;
        }
        mMesh.bLightMap = false;
        mMesh.NumTexuture = 1;
        wcscpy_s(mMesh.Name, L"skybox");



        return S_OK;
    }

    void Sky::Draw()
    {
        // render
        renderer::Renderer::GetInstance()->SetInputLayoutTo(renderer::Renderer::eInputLayout::PT);

        renderer::BufferManager* const bufferManager = renderer::Renderer::GetInstance()->GetBufferManager();
        const renderer::BufferRange vertexRange = bufferManager->GetVertexRangeByHash(mModelHash);
        const renderer::BufferRange indexRange = bufferManager->GetIndexRangeByHash(mModelHash);

        ASSERT((vertexRange.Count >= 0 && vertexRange.StartIndex >= 0), "no matched VertexRange data. hash(%u)", mModelHash);
        ASSERT((indexRange.Count >= 0 && indexRange.StartIndex >= 0), "no matched IndexRange data. hash(%u)", mModelHash);


        uint32 stride = sizeof(renderer::Vertex);
        uint32 offset = vertexRange.StartIndex;

        renderer::Renderer::GetInstance()->BindVertexBuffer(stride, offset);
        renderer::Renderer::GetInstance()->BindIndexBuffer(indexRange.StartIndex);

        renderer::Renderer::GetInstance()->SetRasterState(renderer::Renderer::eRasterType::Skybox);

        renderer::Renderer::GetInstance()->SetShaderTo(renderer::Renderer::eShader::Skybox);

        renderer::Renderer::GetInstance()->BindSamplerToPsByType(0, renderer::Renderer::eSamplerType::AnisotropicWrap);


        renderer::Renderer::GetInstance()->BindCbToVsByType(0U, 1U, renderer::Renderer::eCbType::CbWorld);
        renderer::Renderer::GetInstance()->BindCbToVsByType(1U, 1U, renderer::Renderer::eCbType::CbViewProj);

        renderer::Renderer::GetInstance()->SetDepthStencilState(true);

        // TODO: Renderer로 Bind하도록 이동하기.
        ID3D11DeviceContext* deviceContext = renderer::Renderer::GetInstance()->GetDeviceContext();
        deviceContext->PSSetShaderResources(0, 1, &mMesh.Texture);

        deviceContext->DrawIndexed(static_cast<uint32_t>(mMesh.IndexList.size()), 0, 0);
        SAFETY_RELEASE(deviceContext);

        renderer::Renderer::GetInstance()->SetDepthStencilState(false);

    }

    void Sky::Update()
    {
        // update
        XMFLOAT3 cameraPosition = mCamera->GetCameraPositionFloat();
        XMMATRIX matTranslate = XMMatrixIdentity();
        XMMATRIX matScale = XMMatrixScaling(100.0f, 100.0f, 100.0f);

        matTranslate = XMMatrixTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z);

        mWorld = matScale * matTranslate;

        renderer::Renderer::CbWorld cbWVP;
        cbWVP.Matrix = XMMatrixTranspose(mWorld);
        renderer::Renderer::GetInstance()->UpdateCB(renderer::Renderer::eCbType::CbWorld, &cbWVP);
    }

    /*
     * https://www.braynzarsoft.net/viewtutorial/q16390-20-cube-mapping-skybox
     * 나중에 동적으로 생성하는 sphere 클래스를 만들어두면 좋을 것 같으므로 최대한 건들지 않는 것으로 한다.
     */
    HRESULT Sky::createSphere(uint32 latLines, uint32 lonLines)
    {

        const uint32 numVertex = ((latLines - 2) * lonLines) + 2;
        const uint32 numFace = ((latLines - 3) * (lonLines) * 2) + (lonLines * 2);


        std::vector<renderer::Vertex> vertices(numVertex);

        vertices[0].Position.x = 0.0f;
        vertices[0].Position.y = 0.0f;
        vertices[0].Position.z = 1.0f;

        XMMATRIX matRotationX;
        XMMATRIX matRotationY;
        float sphereYaw = 0.0f;
        float spherePitch = 0.0f;
        XMVECTOR currVertPos = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        for (uint32 i = 0; i < latLines - 2U; ++i)
        {
            spherePitch = (float)(i + 1U) * (3.14f / (float)(latLines - 1U));
            matRotationX = XMMatrixRotationX(spherePitch);
            for (uint32 j = 0U; j < lonLines; ++j)
            {
                sphereYaw = (float)j * (6.28f / (float)lonLines);
                matRotationY = XMMatrixRotationZ(sphereYaw);
                currVertPos = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (matRotationX * matRotationY));
                currVertPos = XMVector3Normalize(currVertPos);
                uint32 index = i * lonLines + j + 1U;
                vertices[index].Position.x = XMVectorGetX(currVertPos);
                vertices[index].Position.y = XMVectorGetY(currVertPos);
                vertices[index].Position.z = XMVectorGetZ(currVertPos);
            }
        }

        vertices[numVertex - 1U].Position.x = 0.0f;
        vertices[numVertex - 1U].Position.y = 0.0f;
        vertices[numVertex - 1U].Position.z = -1.0f;



        std::vector<uint32> indices(numFace * 3U);

        int k = 0;
        for (uint32 l = 0U; l < lonLines - 1U; ++l)
        {
            indices[k] = 0U;
            indices[k + 1] = l + 1U;
            indices[k + 2] = l + 2U;
            k += 3;
        }

        indices[k] = 0U;
        indices[k + 1] = lonLines;
        indices[k + 2] = 1U;
        k += 3;

        for (uint32 i = 0U; i < latLines - 3U; ++i)
        {
            for (uint32 j = 0U; j < lonLines - 1U; ++j)
            {
                indices[k] = i * lonLines + j + 1U;
                indices[k + 1] = i * lonLines + j + 2U;
                indices[k + 2] = (i + 1U) * lonLines + j + 1U;

                indices[k + 3] = (i + 1U) * lonLines + j + 1U;
                indices[k + 4] = i * lonLines + j + 2U;
                indices[k + 5] = (i + 1U) * lonLines + j + 2U;

                k += 6; // next quad
            }

            indices[k] = (i * lonLines) + lonLines;
            indices[k + 1] = (i * lonLines) + 1U;
            indices[k + 2] = ((i + 1U) * lonLines) + lonLines;

            indices[k + 3] = ((i + 1U) * lonLines) + lonLines;
            indices[k + 4] = (i * lonLines) + 1U;
            indices[k + 5] = ((i + 1U) * lonLines) + 1U;

            k += 6;
        }

        for (uint32 l = 0U; l < lonLines - 1U; ++l)
        {
            indices[k] = numVertex - 1U;
            indices[k + 1] = (numVertex - 1U) - (l + 1U);
            indices[k + 2] = (numVertex - 1U) - (l + 2U);
            k += 3;
        }

        indices[k] = numVertex - 1U;
        indices[k + 1] = (numVertex - 1U) - lonLines;
        indices[k + 2] = numVertex - 2U;


        renderer::BufferManager* const bufferManager = renderer::Renderer::GetInstance()->GetBufferManager();
        bufferManager->AddVertexData(reinterpret_cast<int8_t*>(vertices.data()), sizeof(renderer::Vertex) * vertices.size(), mModelHash);
        bufferManager->AddIndexData(reinterpret_cast<int8_t*>(indices.data()), sizeof(uint32_t) * indices.size(), mModelHash);

        mMesh.Vertex.swap(vertices);
        mMesh.IndexList.swap(indices);
        return S_OK;
    }
}
