
[[vk::binding(7, 1)]]
TextureCube cubeTexture : register(t0);

[[vk::binding(13, 1)]]
SamplerState cubeSampler : register(s0);

[[vk::binding(15, 1)]]
cbuffer cbMaterialFactors : register(b0)
{
    float3 Diffuse;
    float Opacity;
    float3 Ambient;
    float Reflectivity;
    float3 Specular;
    float Shininess;
    float3 Emissive;
    float IsLitOn;
}

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
