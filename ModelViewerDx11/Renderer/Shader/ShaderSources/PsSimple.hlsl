
struct PsInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float3 WorldPosition : TEXCOORD2;
};

float4 main(PsInput psInput) : SV_TARGET
{
    return float4(1.0f, 1.0f, 0.0f, 1.0f);
}
