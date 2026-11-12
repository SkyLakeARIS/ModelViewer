
[[vk::binding(7, 1)]]
Texture2D txDiffuse;

[[vk::binding(13, 1)]]
SamplerState texSampler;

struct PsInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PsInput psInput) : SV_TARGET
{
    return txDiffuse.Sample(texSampler, psInput.TexCoord);
}
