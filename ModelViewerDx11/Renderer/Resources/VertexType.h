#pragma once
#include "../../Core/MathPrerequisites.h"

namespace renderer
{
    struct BufferRange
    {
        int32_t StartIndex;
        int32_t Count;
    };

    enum class eBufferUsage : uint8_t
    {
        Static,
        Dynamic,
        UsageCount
    };

    enum class ePrimitiveTopology : uint8_t
    {
        Triangles,
        TriangleStrip,
        Lines,
        TopologyCount
    };

    enum class eVertexFormat : uint8_t
    {
        PTN,    // pos, normal, tex
        PT,     // pos, tex
        P,      // pos
        FormatCount
    };

    struct VertexPTN // 4bytes align
    {
        XMFLOAT3 Position;
        XMFLOAT2 TexCoord;
        XMFLOAT3 Normal;
        float    Reserve1;
    };

    struct VertexPT // 4bytes align
    {
        XMFLOAT3 Position;
        XMFLOAT2 TexCoord;
    };

    struct VertexP // 4bytes align
    {
        XMFLOAT3 Position;
    };

    inline constexpr int16_t GetVertexStrideSize(eVertexFormat vertexAttrib)
    {
        constexpr int16_t VertexStrideMap[static_cast<int8_t>(eVertexFormat::FormatCount)] =
        {
            sizeof(VertexPTN),
            sizeof(VertexPT),
            sizeof(VertexP)
        };
        return VertexStrideMap[static_cast<int8_t>(vertexAttrib)];
    }
}
