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

cbuffer CbMatLightViewProj : register(b2)
{
    matrix MatLightViewProj;
}

cbuffer CbLightProperty : register(b3)
{
    float4 vLightColor;
    float4 vLightDir;
}

cbuffer CbCamera : register(b4)
{
    float3 Position;
    float Reserve;
}

struct VsInput
{
    [[vk::location(0)]] float4 Position : POSITION;
    [[vk::location(1)]] float2 TexCoord : TEXCOORD0;
};

struct PsInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    nointerpolation float3 CameraPosition : TEXCOORD1;
    nointerpolation float4 LightColor : TEXCOORD2;
    nointerpolation float4 LightDir : TEXCOORD3;
    nointerpolation matrix MatLightViewProj : TEXCOORD4;
};

PsInput main(VsInput vsInput)
{
    PsInput psInput;
    psInput.Position = vsInput.Position;
    psInput.TexCoord = vsInput.TexCoord;
    psInput.CameraPosition = Position.xyz;
    psInput.LightColor = vLightColor;
    psInput.LightDir = vLightDir;
    psInput.MatLightViewProj = MatLightViewProj;
	return psInput;
}
