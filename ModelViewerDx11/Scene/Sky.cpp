#include "Sky.h"
#include "Camera.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Resources/BufferManager.h"
#include "../Renderer/Resources/TextureManager.h"
#include "../Util/Define.h"
#include "../Util/Macro.h"
#include "../Util/Util.h"

namespace scene
{
    Sky::Sky(Camera& camera)
        : mCamera(&camera)
        , mMesh()
        , mWorld(XMMatrixIdentity())
        , mLatLines(0)
        , mLonLines(0)
    {


    }

    Sky::~Sky()
    {
        mCamera = nullptr;
        // TODO: 추가한 BufferData 처리할 수 있는 로직이 필요함. - (종료될 떄 처리되기 때문에 당장 문제는 없음)
    }

    HRESULT Sky::Initialize(uint32 latLines, uint32 lonLines, renderer::TextureManager* const texManager, renderer::Renderer& renderer)
    {
        mLonLines = lonLines;
        mLatLines = latLines;
        int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
        const int16_t wroteCount = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Sphere_%d_%d.mesh",
            reinterpret_cast<const char*>(renderer::VIRTUAL_ROOT_PATH), mLonLines, mLatLines);

        mMesh.MeshHash = util::GetDjb2Hash(virtualFilePath);
        (void)memcpy(mMesh.MeshName, virtualFilePath, wroteCount + 1);
        // init mesh info
        HRESULT result;
        result = createSphere(latLines, lonLines, renderer);
        if(FAILED(result))
        {
            return result;
        }
        
        const int8_t* const filePath = reinterpret_cast<const int8_t*>("./AssetData/textures/skybox.dds");
        texManager->AddDTextureDDS(filePath, mMesh.TextureHashes[static_cast<int8_t>(renderer::eTextureType::Diffuse)]);
        if (FAILED(result))
        {
            ASSERT(false, "Skybox - fail to create texture");
            return result;
        }
        return S_OK;
    }

    void Sky::Draw(renderer::Renderer& renderer)
    {
        // render
        renderer.BindInputLayoutTo(renderer::eInputLayout::PT);




        const int16_t strideVertex = renderer::GetVertexStrideSize(mMesh.VertexLayoutType);

        renderer.BindVertexBufferNew(strideVertex, 0);
        renderer.BindIndexBufferNew(0);

        renderer.BindRasterStateByType(renderer::eRasterType::Skybox);

        renderer.BindShaderTo(renderer::eShader::Skybox);

        renderer.BindSamplerToPsByType(0, renderer::eSamplerType::AnisotropicWrap);


        renderer.BindCbToVsByType(0U, 1U, renderer::eCbType::CbWorld);
        renderer.BindCbToVsByType(1U, 1U, renderer::eCbType::CbViewProj);

        renderer.BindDepthStencilState(true);

        renderer.BindTextureToPs(0, mMesh.TextureHashes[static_cast<int8_t>(renderer::eTextureType::Diffuse)]);

        renderer.DrawIndexed(mMesh.IndexRange.Count, mMesh.IndexRange.StartIndex, mMesh.VertexRange.StartIndex);

        renderer.BindDepthStencilState(false);
    }

    void Sky::Update(renderer::Renderer& renderer)
    {
        // update
        XMFLOAT3 cameraPosition = mCamera->GetCameraPositionFloat();
        XMMATRIX matTranslate = XMMatrixIdentity();
        XMMATRIX matScale = XMMatrixScaling(100.0f, 100.0f, 100.0f);

        matTranslate = XMMatrixTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z);

        mWorld = matScale * matTranslate;

        renderer::CbWorld cbWVP;
        cbWVP.Matrix = XMMatrixTranspose(mWorld);
        renderer.UpdateCB(renderer::eCbType::CbWorld, &cbWVP);
    }

    /*
     * https://www.braynzarsoft.net/viewtutorial/q16390-20-cube-mapping-skybox
     * 나중에 동적으로 생성하는 sphere 클래스를 만들어두면 좋을 것 같으므로 최대한 건들지 않는 것으로 한다.
     */
    HRESULT Sky::createSphere(uint32 latLines, uint32 lonLines, renderer::Renderer& renderer)
    {
        const uint32 numVertex = ((latLines - 2) * lonLines) + 2;
        const uint32 numFace = ((latLines - 3) * (lonLines) * 2) + (lonLines * 2);

        std::vector<renderer::VertexPT> vertices(numVertex);

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


        renderer::BufferManager* const bufferManager = renderer.GetBufferManager();

        mMesh.VertexLayoutType = renderer::eInputLayout::PT;
        const int16_t strideVertex = renderer::GetVertexStrideSize(mMesh.VertexLayoutType);
        const int16_t strideIndex = bufferManager->GetIndexStrideSize();

        bufferManager->AddVertexData(reinterpret_cast<int8_t*>(vertices.data()), strideVertex * vertices.size(), mMesh.MeshHash, strideVertex, mMesh.VertexRange);
        bufferManager->AddIndexData(reinterpret_cast<int8_t*>(indices.data()), strideIndex * indices.size(), mMesh.MeshHash, strideIndex, mMesh.IndexRange);

        return S_OK;
    }
}
