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
};

PsInput main(VsInput vsInput)
{
    PsInput psInput;
    psInput.Position = mul(vsInput.Position, MatWorld.Matrix);
    psInput.Position = mul(psInput.Position, MatLightViewProj);
    return psInput;
}
