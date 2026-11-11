/*
    set 0, binding 1 : 프레임 단위 통합 상수 (PerFrameUniformBlock과 순서·크기 일치)
    World Matrix는 push constant
 */

struct CbMatrix
{
    matrix Matrix;
};

[[vk::push_constant]] CbMatrix MatWorld;

[[vk::binding(1, 0)]]
cbuffer CbPerFrame
{
    matrix MatViewProj;
    matrix MatLightViewProj;
    float4 vLightColor;
    float4 vLightDir;
    float3 CameraPosition;
    float  Reserve;
    float4 Reserve2;
    matrix MatOrthographic;
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
    float3 WorldNormal : TEXCOORD1;
    float3 WorldPosition : TEXCOORD2;
    float4 LightClipPosition : TEXCOORD3;
    nointerpolation float4 LightColor : TEXCOORD4;
    nointerpolation float4 LightDir : TEXCOORD5;
    nointerpolation float3 CameraPosition : TEXCOORD6;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PsInput main(VsInput vsInput)
{
    PsInput psInput;

    psInput.Position = mul(vsInput.Position, MatWorld.Matrix);
    psInput.WorldNormal = mul(float4(vsInput.Normal, 0.0f), MatWorld.Matrix).xyz;
    psInput.WorldPosition = psInput.Position.xyz;
    psInput.LightClipPosition = mul(psInput.Position, MatLightViewProj);
    psInput.Position = mul(psInput.Position, MatViewProj);

    psInput.TexCoord = vsInput.TexCoord;

    psInput.LightColor = vLightColor;
    psInput.LightDir = vLightDir;
    psInput.CameraPosition = CameraPosition;

    return psInput;
}
