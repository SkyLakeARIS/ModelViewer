
[[vk::binding(7, 1)]]
Texture2D txDiffuse;

[[vk::binding(13, 1)]]
SamplerState texSampler;

[[vk::binding(15, 1)]]
cbuffer cbMaterialFactors
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
    float3 WorldPosition : TEXCOORD2;
};

struct PsOutput
{
    float4 Color : SV_TARGET0;      // w - reserved
    float4 Normal : SV_TARGET1;     // w - reserved
    float4 Position : SV_TARGET2;    // w - Lit On/Off
    float4 Specular : SV_TARGET3;   // w - Shininess
    float4 Ambient : SV_TARGET4;     // w - reserved
};

PsOutput main(PsInput psInput)
{
    PsOutput psOutput;
    psOutput.Color = txDiffuse.Sample(texSampler, psInput.TexCoord);
    const float Reserved = 0.0f;
    psOutput.Normal = float4(1.0f, 0.0f, 0.0f, Reserved);
    psOutput.Position = float4(psInput.WorldPosition, IsLitOn);
    psOutput.Specular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    psOutput.Ambient = float4(0.0f, 0.0f, 0.0f, Reserved);
    return psOutput;
}
