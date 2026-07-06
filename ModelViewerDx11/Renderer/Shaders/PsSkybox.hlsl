
TextureCube cubeTexture : register(t0);

SamplerState cubeSampler : register(s0);

struct PsInput
{
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD0;
};

float4 main(PsInput input) : SV_TARGET
{
    return cubeTexture.Sample(cubeSampler, input.TexCoord);
}