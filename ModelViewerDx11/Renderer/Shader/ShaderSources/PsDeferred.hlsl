[[vk::binding(7, 1)]]
Texture2D texGColor;
[[vk::binding(8, 1)]]
Texture2D texGNormal;
[[vk::binding(9, 1)]]
Texture2D texGPosition;
[[vk::binding(10, 1)]]
Texture2D texGSpecular;
[[vk::binding(11, 1)]]
Texture2D texGAmbient;
[[vk::binding(12, 1)]]
Texture2D texShadow;

[[vk::binding(13, 1)]]
SamplerState texSampler;


struct PsInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    nointerpolation float3 CameraPosition : TEXCOORD1;
    nointerpolation float4 LightColor : TEXCOORD2;
    nointerpolation float4 LightDir : TEXCOORD3;
    nointerpolation matrix MatLightViewProj : TEXCOORD4;
};

float4 main(PsInput psInput) : SV_TARGET
{
    float3 outColor = texGColor.Sample(texSampler, psInput.TexCoord).rgb;
    const float3 worldNormal = normalize(texGNormal.Sample(texSampler, psInput.TexCoord).xyz);
    const float4 worldPosition = texGPosition.Sample(texSampler, psInput.TexCoord);
    const float IsLitOn = worldPosition.w;
    const float4 specular = texGSpecular.Sample(texSampler, psInput.TexCoord);
    const float3 ambient = texGAmbient.Sample(texSampler, psInput.TexCoord).rgb;

    // MEMO: Lit On인 픽셀만
    if (IsLitOn > 0.0f)
    {
        float4 lightClipSpace = mul(float4(worldPosition.xyz, 1.0f), psInput.MatLightViewProj);
        lightClipSpace = lightClipSpace / lightClipSpace.w;
        const float2 shadowTexCoord = float2(lightClipSpace.xy) * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
        const float depthInShadowMap = texShadow.Sample(texSampler, shadowTexCoord).r;

        const float DEPTH_OFFSET = 0.00001f;
        if (lightClipSpace.z > depthInShadowMap + DEPTH_OFFSET)
        {
            // MEMO: ambient 색상을 그림자 색상으로 사용하는 것이 시도한 것 중 가장 자연스러움
            outColor *= ambient * psInput.LightColor.rgb;
        }
        else
        {
            const float diffuseIntensity = saturate(dot(normalize(-psInput.LightDir.xyz), worldNormal));
            float specularIntensity = 0.0f;
            if (diffuseIntensity > 0.0f)
            {
                const float3 reflectLight = normalize(reflect(psInput.LightDir.xyz, worldNormal));
                // MEMO: 미리 뒤집어서 사용
                const float3 cameraDir = normalize(psInput.CameraPosition - worldPosition.xyz);
                specularIntensity = dot(reflectLight, cameraDir);
                specularIntensity = pow(saturate(specularIntensity), specular.w);
            }
            outColor *= (diffuseIntensity + specular.xyz * specularIntensity + ambient) * psInput.LightColor.rgb;
        }
    }
    return float4(outColor, 1.0f);
}
