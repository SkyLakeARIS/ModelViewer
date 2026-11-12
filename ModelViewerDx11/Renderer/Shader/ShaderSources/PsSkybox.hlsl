
[[vk::binding(7, 1)]]
TextureCube cubeTexture;

[[vk::binding(13, 1)]]
SamplerState cubeSampler;

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

struct PsInput
{
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD0;
    float3 WorldPosition : TEXCOORD2;
};

float4 main(PsInput psInput) : SV_TARGET
{
    return cubeTexture.Sample(cubeSampler, psInput.TexCoord);
}
