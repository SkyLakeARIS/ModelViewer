/*
    b0 ~ b5 는 렌더러에서 예약 (reserved slots by renderer)

    b0 : World Matrix
    b1 : View + Projection Matrix (mainly Perspective)
    b2 : View + Projection Matrix of Light
    b3 : Light Attributes (for lighting)
    b4 : Camera Attribute (for lighting)
    b5 : Orthographic Matrix (for screen)
 */

cbuffer CbMatWorld : register(b0)
{
    matrix MatWorld;
}

cbuffer CbMatViewProj : register(b1)
{
    matrix MatViewProj;
}

cbuffer CbOutline : register(b6)
{
    float OutlineWidth;
}

struct VsInput
{
    [[vk::location(0)]] float4 Position : POSITION;
    [[vk::location(1)]] float2 TexCoord : TEXCOORD0;
    [[vk::location(2)]] float3 Normal : NORMAL;
};

struct PsInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

PsInput main(VsInput vsInput)
{
    PsInput psInput;
    float outlineWidth = 0.01f;
    psInput.Position = mul(float4(vsInput.Normal.xyz * outlineWidth + vsInput.Position.xyz, 1.0f), MatWorld);
    psInput.Position = mul(float4(psInput.Position.xyz, 1.0f), MatViewProj);
    psInput.TexCoord = vsInput.TexCoord;
	return psInput;
}
