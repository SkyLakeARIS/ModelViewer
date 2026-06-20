#pragma once
#include "../../framework.h"

namespace renderer
{
    struct Vertex // 4bytes align
    {
        XMFLOAT3 Position;
        XMFLOAT2 TexCoord;
        XMFLOAT3 Normal;
        float    Reserve1;
    };

    struct VertexTex // 4bytes align
    {
        XMFLOAT3 Position;
        XMFLOAT2 TexCoord;
    };

    // 모델링 프로그램에서 미리 계산된 값으로 사용
    struct Material // 16 bytes align
    {
        XMFLOAT3 Diffuse;
        float    Reserve0;
        XMFLOAT3 Ambient;
        float    Reserve1;
        XMFLOAT3 Specular;
        float    Reserve2;
        XMFLOAT3 Emissive;
        float    Reserve3;
        float    Opacity;       // 알파값으로 사용
        float    Reflectivity;
        float    Shininess;     // 스페큘러 거듭제곱 값
        float    Reserve4;
    };
    typedef Material CbMaterial;

    struct BufferRange
    {
        int32_t StartIndex;
        int32_t Count;
    };

    typedef struct CbMatrix
    {
        XMMATRIX Matrix;
    }CbWorld, CbViewProj, CbLightViewProjMatrix;

    typedef struct CbFloat3
    {
        XMFLOAT3    Float3;
        float       Reserve;
    } CbCameraPosition, CbOutlineProperty, CbColor;

    typedef struct CbTwoVec4
    {
        XMFLOAT4    First;
        XMFLOAT4    Second;
    }CbLightProperty;

    enum class eCbType : uint8_t
    {
        CbWorld,
        CbViewProj,
        CbLightViewProjMatrix,
        CbCameraPosition,
        CbOutlineProperty,
        CbLightProperty,
        CbMaterial,
        CbColor,
        ConstantBufferCount
    };

    enum class eRasterType
    {
        Basic,
        Outline,
        Skybox,
        CullBack,
        RasterCount,
    };

    enum class eSamplerType
    {
        AnisotropicWrap,
        SamplerCount
    };

    enum class eInputLayout : uint8_t
    {
        PTN,    // pos, normal, tex
        PT,     // pos, tex
        P,      // pos
        InputlayoutCount
    };

    enum class eShader : uint32_t
    {
        Outline,
        Skybox,
        Shadow,
        BasicWithShadow,
        RenderToTexture,
        Color,
        ShaderCount
    };

    // RenderTarget, DepthStencil 
    enum class eRenderTarget : uint8_t
    {
        Default,
        Shadow,
        RenderTargetCount
    };

}
