/*
    set 0, binding 1 : 프레임 단위 통합 상수 (PerFrameUniformBlock과 순서·크기 일치)
    World Matrix는 push constant
 */

struct PerObject
{
    matrix WorldMatrix;
    float3 Diffuse;
    float Opacity;   // 알파값으로 사용
    float3 Ambient;
    float Reflectivity;
    float3 Specular;  // 스페큘러 거듭제곱 값
    float Shininess;
    float3 Emissive;
    float IsLitOn;
};

[[vk::push_constant]] PerObject pcPerObject;

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
    psInput.Position = mul(vsInput.Position, pcPerObject.WorldMatrix);
    psInput.Position = mul(psInput.Position, MatLightViewProj);
    return psInput;
}
