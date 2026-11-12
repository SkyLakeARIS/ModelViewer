//--------------------------------------------------------------------------------------
// Pixel Shader
// Color + 라이팅 (포워드 패스). PsBasicWithShadow 기반, 텍스처 대신 머티리얼 Diffuse를 기본 색으로 사용
//--------------------------------------------------------------------------------------
[[vk::binding(2, 0)]]
Texture2D shadowMap;

[[vk::binding(3, 0)]]
SamplerState shadowSampler;

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
    float2 TexCoord : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float3 WorldPosition : TEXCOORD2;
    float4 LightClipPosition : TEXCOORD3;
    nointerpolation float4 LightColor : TEXCOORD4;
    nointerpolation float4 LightDir : TEXCOORD5;
    nointerpolation float3 CameraPosition : TEXCOORD6;
};


float4 main(PsInput psInput) : SV_TARGET
{
    float3 outColor = pcPerObject.Diffuse;

    // MEMO: Lit On인 픽셀만
    if (pcPerObject.IsLitOn > 0.0f)
    {
        const float3 lightNdc = psInput.LightClipPosition.xyz / psInput.LightClipPosition.w;
        const float2 shadowTexCoord = lightNdc.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
        const float depthInShadowMap = shadowMap.Sample(shadowSampler, shadowTexCoord).r;

        const float DEPTH_OFFSET = 0.00001f;
        if (lightNdc.z > depthInShadowMap + DEPTH_OFFSET)
        {
            // MEMO: ambient 색상을 그림자 색상으로 사용하는 것이 시도한 것 중 가장 자연스러움
            outColor *= pcPerObject.Ambient * psInput.LightColor.rgb;
        }
        else
        {
            const float3 worldNormal = normalize(psInput.WorldNormal);
            const float diffuseIntensity = saturate(dot(normalize(-psInput.LightDir.xyz), worldNormal));
            float specularIntensity = 0.0f;
            if (diffuseIntensity > 0.0f)
            {
                const float3 reflectLight = normalize(reflect(psInput.LightDir.xyz, worldNormal));
                // MEMO: 미리 뒤집어서 사용
                const float3 cameraDir = normalize(psInput.CameraPosition - psInput.WorldPosition);
                specularIntensity = dot(reflectLight, cameraDir);
                specularIntensity = pow(saturate(specularIntensity), pcPerObject.Shininess);
            }
            outColor *= (diffuseIntensity + pcPerObject.Specular * specularIntensity + pcPerObject.Ambient) * psInput.LightColor.rgb;
        }
    }

    return float4(outColor, pcPerObject.Opacity);
}
