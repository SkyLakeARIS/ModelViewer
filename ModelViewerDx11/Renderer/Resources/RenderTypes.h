#pragma once
#include "TextureData.h"
#include "../Shader/ShaderType.h"



namespace renderer
{
    struct PerFrameUniformBlock
    {
        XMMATRIX MatViewProj;
        XMMATRIX MatLightViewProj;
        XMFLOAT4 vLightColor;
        XMFLOAT4 vLightDir;
        XMFLOAT3 CameraPosition;
        float Reserve;
        XMFLOAT4 Reserve2;
        XMMATRIX MatOrthographic;
    };

    enum class eRasterType : uint8_t
    {
        Basic,
        Outline,
        Skybox,
        CullBack,
        RasterCount,
    };

    enum class eSamplerType : uint8_t
    {
        Linear,
        AnisotropicWrap,
        SamplerCount
    };

    enum class eRenderPass : uint8_t
    {
        Main,
        Shadow,
        UI,
        Deferred, // MEMO: renderer only
        PassCount
    };

    // MEMO: 우선은 캐시 변수가 unbind 상태로 초기화 될 수 있도록 열거형으로 변경
    enum class eDepthStencilState
    {
        // MEMO: DepthOffStencilOff는 unbind용이나 다름 없음.
        DepthOffStencilOff,
        DepthOnMaskAllCompLessEqual,
        StateCount
    };

    enum eBlendState : uint8_t
    {
        Opaque,
        AlphaBlend,
        StateCount
    };

}
