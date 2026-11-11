[[vk::binding(2, 0)]]
Texture2D shadowMap;
[[vk::binding(3, 0)]]
SamplerState shadowSampler;

[[vk::binding(7, 1)]]
Texture2D txDiffuse : register(t0);
[[vk::binding(13, 1)]]
SamplerState texSampler : register(s0);

struct PsInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float3 WorldPosition : TEXCOORD2;
    float4 LightClipPosition : TEXCOORD3;
    nointerpolation float4 LightColor : TEXCOORD4;
};

float4 main(PsInput psInput) : SV_TARGET
{
    float4 outColor = txDiffuse.Sample(texSampler, psInput.TexCoord);

    const float3 lightNdc = psInput.LightClipPosition.xyz / psInput.LightClipPosition.w;
    const float2 shadowTexCoord = lightNdc.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    const float depthInShadowMap = shadowMap.Sample(shadowSampler, shadowTexCoord).r;

    const float DEPTH_OFFSET = 0.00001f;
    if (lightNdc.z > depthInShadowMap + DEPTH_OFFSET)
    {
        // MEMO: 머티리얼이 없으니 고정 ambient를 그림자 색상으로 사용
        const float3 SHADOW_AMBIENT = float3(0.4f, 0.4f, 0.4f);
        outColor.rgb *= SHADOW_AMBIENT * psInput.LightColor.rgb;
    }

    return outColor;
}
