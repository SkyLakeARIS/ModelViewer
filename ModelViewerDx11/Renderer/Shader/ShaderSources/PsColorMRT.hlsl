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
    float3 WorldNormal : TEXCOORD1;
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
    const float Reserved = 0.0f;
    psOutput.Color = float4(Diffuse, Reserved);
    psOutput.Normal = float4(psInput.WorldNormal, Reserved);
    psOutput.Position = float4(psInput.WorldPosition, IsLitOn);
    psOutput.Specular = float4(Specular, Shininess);
    psOutput.Ambient = float4(Ambient, Reserved);
    return psOutput;
}
