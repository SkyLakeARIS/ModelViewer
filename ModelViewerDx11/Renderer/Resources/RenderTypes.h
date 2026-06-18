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
}
