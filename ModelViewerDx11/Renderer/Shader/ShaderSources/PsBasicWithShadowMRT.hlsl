[[vk::binding(7, 1)]]
Texture2D texModel : register(t0);

[[vk::binding(13, 1)]]
SamplerState texSampler : register(s0);


[[vk::binding(15, 1)]]
cbuffer cbMaterialFactors : register(b0)
{
    float3 Diffuse;
    float Opacity;   // 알파값으로 사용
    float3 Ambient;
    float Reflectivity;
    float3 Specular;  // 스페큘러 거듭제곱 값
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
    float4 Position: SV_TARGET2;    // w - Lit On/Off
    float4 Specular : SV_TARGET3;   // w - Shininess
    float4 Ambient: SV_TARGET4;     // w - reserved
};


PsOutput main(PsInput psInput)
{
    PsOutput psOutput;

    const float Reserved = 0.0f;
    psOutput.Color = texModel.Sample(texSampler, psInput.TexCoord);
    psOutput.Position = float4(psInput.WorldPosition, IsLitOn);
    psOutput.Normal = float4(psInput.WorldNormal, Reserved);
    psOutput.Specular = float4(Specular, Shininess);
    psOutput.Ambient = float4(Ambient, Reserved);

    return psOutput;
}
