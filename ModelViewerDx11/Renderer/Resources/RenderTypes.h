#pragma once
#include "TextureData.h"
#include "VertexType.h"
#include "../Shader/ShaderType.h"



namespace renderer
{
    typedef struct CbMatrix
    {
        XMMATRIX Matrix;
    }CbWorld, CbViewProj, CbLightViewProjMatrix, CbScreenSpaceMatrix;

    typedef struct CbFloat
    {
        float    Float;
        XMFLOAT3 Reserve;
    } CbOutlineProperty;

    typedef struct CbFloat3
    {
        XMFLOAT3    Float3;
        float       Reserve;
    } CbCameraPosition, CbColor;

    typedef struct CbTwoVec4
    {
        XMFLOAT4    First;
        XMFLOAT4    Second;
    }CbLightProperty;

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

    // RenderTarget, DepthStencil 
    enum class eRenderTarget : uint8_t
    {
        Default,
        Shadow,
        GBufferColor,
        GBufferNormal,
        GBufferPosition,
        GBufferSpecular,
        GBufferAmbient,
        RenderTargetCount
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

    enum class eShadowMapUsage
    {
        Off,
        On,
        UsageCount
    };

    enum eBlendState : uint8_t
    {
        Opaque,
        AlphaBlend,
        StateCount
    };

    struct RenderState
    {
        // MEMO: 셰이더
        eShader ShaderType;
        int8_t TexBindingSlots[static_cast<uint8_t>(eTextureType::TextureTypeCount)];
        int8_t SamplerBindingSlot;
        // MEMO: 렌더 상태
        eRasterType RasterType;
        eSamplerType SamplerType;
        ePrimitiveTopology TopologyType;
        eBlendState BlendState;
        eDepthStencilState DepthStencilState;
        eShadowMapUsage UseShadowMapUsage;
    };
}
