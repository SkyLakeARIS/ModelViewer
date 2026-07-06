
struct PsInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PsInput input) : SV_TARGET
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
