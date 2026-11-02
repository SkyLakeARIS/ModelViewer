
[[vk::binding(7, 1)]]
Texture2D txDiffuse : register(t0);

[[vk::binding(13, 1)]]
SamplerState texSampler : register(s0);

struct PsInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PsInput psInput) : SV_TARGET
{
    return txDiffuse.Sample(texSampler, psInput.TexCoord);
}
