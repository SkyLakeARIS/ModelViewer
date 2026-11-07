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
    float2 TexCoord : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float3 WorldPosition : TEXCOORD2;
};

float4 main(PsInput psInput) : SV_TARGET
{
    return float4(Diffuse, Opacity);
}
