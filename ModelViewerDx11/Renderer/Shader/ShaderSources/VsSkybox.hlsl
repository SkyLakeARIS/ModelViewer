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
cbuffer CbPerFrame : register(b1)
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
};

struct PsInput
{
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD0;
    float3 WorldPosition : TEXCOORD2;
};

PsInput main(VsInput vsInput)
{
    PsInput psInput;

    // 깊이 값이 가장 뒤어야 하기 때문에 따로 다시 지정하기 보다 1인 w값을 대신 쓰는 것. w = 1, using w instead of z
    //output.Position = mul(float4(input.Position.xyz, 1), MatWorld.Matrix).xyww;
    psInput.Position = mul(vsInput.Position, MatWorld.Matrix);
    psInput.WorldPosition = psInput.Position.xyz;
    psInput.Position = mul(float4(psInput.Position.xyz, 1.0f), MatViewProj).xyww;

    // tex값은 위치 값
    psInput.TexCoord = vsInput.Position.xyz;
    return psInput;
}
