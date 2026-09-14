#include "Light.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Primitive/MeshGenerator.h"
#include "../Renderer/Resources/BufferManager.h"
#include "../Renderer/Resources/RenderPacket.h"
#include "../Util/Util.h"

namespace scene
{
    Light::Light(XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 color, float nearPlane, float farPlane)
        : mPosition(pos)
        , mDirection(dir)
        , mColor(color)
        , mMatView()
        , mMatProj(XMMatrixIdentity())
        , mMatViewProj(XMMatrixIdentity())
        , mMeshDebug()
        , mNearPlane(nearPlane)
        , mFarPlane(farPlane)
    {
        mLines.reserve(24);
    }

    Light::~Light()
    {
    }

    void Light::SubmitDebugCommand(std::vector<renderer::RenderPacket>& commandList)
    {
        for (const auto& subMesh : mMeshDebug.SubMeshes)
        {
            renderer::RenderPacket command = renderer::RenderPacket::MakeCommand(
                mMeshDebug.VertexFormat,
                renderer::GetVertexStrideSize(mMeshDebug.VertexFormat),
                renderer::eBufferUsage::Dynamic,
                false,
                subMesh.VertexRange,
                subMesh.IndexRange,
                subMesh.Material,
                renderer::eRenderPass::Main,
                XMMatrixIdentity(),
                renderer::eShader::DebugColor,
                renderer::eRasterType::Basic,
                renderer::eSamplerType::SamplerCount,
                renderer::eBlendState::Opaque,
                renderer::ePrimitiveTopology::Lines,
                false,
                renderer::eDepthStencilState::DepthOffStencilOff
            );

            commandList.push_back(command);
        }
    }

    void Light::Update(renderer::Renderer& renderer)
    {
        XMVECTOR vPosition = XMLoadFloat3(&mPosition);
        XMVECTOR vDirection = XMLoadFloat3(&mDirection);
        XMFLOAT3 lookAt;
        XMStoreFloat3(&lookAt, XMVectorAdd(vPosition, vDirection));
        mMatView = XMMatrixLookAtLH(XMLoadFloat3(&mPosition), XMLoadFloat3(&lookAt), XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));
        mMatProj = XMMatrixOrthographicLH(-10.0f, 10.0f, mNearPlane, mFarPlane);
        mMatViewProj = mMatView * mMatProj;

        // MEMO: 디버그 용 프러스텀 라인 데이터 생성
        constexpr XMFLOAT3 PointsInNDC[8] = {
            {-1.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {-1.0, -1.0, 0.0}, {1.0, -1.0, 0.0},
            {-1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {-1.0, -1.0, 1.0}, {1.0, -1.0, 1.0}
        };
        XMFLOAT3 pointToWorld[8] = {
        };

        XMMATRIX lightViewProjInv = XMMatrixInverse(nullptr, (mMatView * mMatProj));
        // move points light view space to world space
        for (uint32_t i = 0; i < 8; ++i)
        {
            XMVECTOR vecPointToWorld = XMLoadFloat3(&PointsInNDC[i]);
            vecPointToWorld          = XMVector3Transform(vecPointToWorld, lightViewProjInv);
            const XMVECTOR vecDividerW = XMVectorReplicate(vecPointToWorld.m128_f32[3]);
            vecPointToWorld = XMVectorDivide(vecPointToWorld, vecDividerW);
            XMStoreFloat3(&pointToWorld[i], vecPointToWorld);
        }

        mLines.clear();

        mLines.push_back(mPosition);
        mLines.push_back(mDirection);

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

        if (mMeshDebug.MeshHash == 0)
        {
            int8_t virtualFilePath[util::MAX_PATH_LENGTH] = {};
            const int32_t wroteCount = sprintf_s(reinterpret_cast<char*>(virtualFilePath), util::MAX_PATH_LENGTH, "%sPrimitive_Light_Debug_Line.mesh",
                reinterpret_cast<const char*>(renderer::MeshGenerator::VIRTUAL_ROOT_PATH));

            (void)memcpy(mMeshDebug.MeshName, virtualFilePath, wroteCount + 1);

            mMeshDebug.MeshHash = util::GetDjb2Hash(virtualFilePath);
            mMeshDebug.VertexFormat = renderer::eVertexFormat::P;
        }

        if (mMeshDebug.SubMeshes.empty())
        {
            // no Sampler, Blend
            renderer::SubMesh newSubMesh = {};

            (void)memcpy(newSubMesh.SubMeshName, mMeshDebug.MeshName, util::MAX_NAME_LENGTH);
            newSubMesh.SubMeshHash = mMeshDebug.MeshHash;
            newSubMesh.Material.Factors.Diffuse = XMFLOAT3(1.0f, 1.0f, 0.0f);
            newSubMesh.Material.Factors.IsLitOn = static_cast<float>(false);

            mMeshDebug.SubMeshes.push_back(std::move(newSubMesh));
        }

        renderer::SubMesh& subMesh = mMeshDebug.SubMeshes.front();

        const int16_t strideVertex = renderer::GetVertexStrideSize(mMeshDebug.VertexFormat);
        renderer::BufferManager* const bufferManager = renderer.GetBufferManager();
        bufferManager->AddVertexDynamic(reinterpret_cast<int8_t*>(mLines.data()), static_cast<uint32_t>(strideVertex) * static_cast<uint32_t>(mLines.size()), subMesh.SubMeshHash, strideVertex, subMesh.VertexRange);

        mMeshDebug.VertexRange = subMesh.VertexRange;
    }

    XMFLOAT4 Light::GetDirection() const
    {
        return XMFLOAT4(mDirection.x, mDirection.y, mDirection.z, 1.0f);
    }

    XMFLOAT3 Light::GetPosition() const
    {
        return mPosition;
    }

    XMFLOAT4 Light::GetColor() const
    {
        return XMFLOAT4(mColor.x, mColor.y, mColor.z, 1.0f);
    }

    XMMATRIX Light::GetViewProjMatrix() const
    {
        return mMatViewProj;
    }

}
