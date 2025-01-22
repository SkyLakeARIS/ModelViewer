//--------------------------------------------------------------------------------------
// Pixel Shader
// 여러개 만들어도 된다. 컴파일 할 때 함수명만 잘 지정해두면. (여러 셰이더 컴파일 해두고, blob만 바꿔서 런타임에 쓰도록 하는것?)
//--------------------------------------------------------------------------------------
Texture2D texModel : register(t0);
Texture2D texNormal : register(t1);
Texture2D texShadow : register(t2);

SamplerState samLinear : register(s0);


cbuffer cbMaterial : register(b0)
{
    float3 Diffuse;
    float Reserve0;
    float3 Ambient;
    float Reserve1;
    float3 Specular;
    float Reserve2;
    float3 Emissive;
    float Reserve3;
    float Opacity; // 알파값으로 사용
    float Reflectivity;
    float Shininess; // 스페큘러 거듭제곱 값
    float Reserve4;
}

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
    float3 LightColor : TEXCOORD1;
    float3 LightDir : TEXCOORD2;
    float3 CameraDir : TEXCOORD3;
    float4 ClipPosition: TEXCOORD4;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 finalColor = texModel.Sample(samLinear, input.UV);
    float3 normal = texNormal.Sample(samLinear, input.UV); // face는 _N 텍스쳐가 없음.

    float3 shadowCoord = input.ClipPosition.xyz / input.ClipPosition.w;
    float depth = shadowCoord.z;

   // shadowCoord.y = -shadowCoord.y;
    shadowCoord.x = shadowCoord.x * 0.5 + 0.5;
    shadowCoord.y = -(shadowCoord.y * 0.5) + 0.5;
    float depthShadow = texShadow.Sample(samLinear, shadowCoord.xy).r;

   // if (depth < depthShadow - 0.0001)
    if (depth < depthShadow - 0.0001)
    {
        normal.z = sqrt(1.0 - (normal.x * normal.x + normal.y * normal.y));

    // 1. VS에서 계산한 노말값 사용할 때
        float3 diffuse = dot(normalize(normal), -normalize(input.LightDir));
    // 2. 텍스쳐가 노말맵이라고 가정하고 사용할 때
    //float3 diffuse = dot(normal, -input.LightDir);
        float3 specular = float3(0.0f, 0.0f, 0.0f);
        if (diffuse.x > 0)
        {
            float3 reflection = normalize(reflect(input.LightDir, normalize(normal)));
            specular = saturate(dot(reflection, normalize(-input.CameraDir)));
            specular = pow(specular, Shininess) * Reflectivity;
            specular *= Specular * input.LightColor.xyz;
        }

        diffuse = saturate(diffuse) * input.LightColor.xyz * Reflectivity;

        float3 ambient = Ambient * input.LightColor;
   // finalColor.xyz *= (specular + diffuse + ambient + Emissive); // 빛나는 효과 == Emissive. 켜고 끌 수 있는 기능 추가 필요
        finalColor.xyz *= (specular + diffuse + ambient);
        finalColor.a = Opacity;
    }
    else // shadow
    {
        finalColor.xyz *= float3(0.4f, 0.4f, 0.4f);
        finalColor.a = Opacity;
    }
    finalColor.xyz = finalColor.xyz / (finalColor.xyz + 1);
    return finalColor;
}