cbuffer cbMatrix : register(b0)
{
    matrix MatWorld;
}

cbuffer cbOutline : register(b1)
{
    float OutlineWidth;
}

cbuffer cbMatrix : register(b2)
{
    matrix MatViewProj;
}

struct VsInput
{
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal : NORMAL;
};

struct PsInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

PsInput main(VsInput input)
{
    PsInput output;
    float outlineWidth = 0.01f;
    output.Position = mul(float4(input.Normal.xyz * outlineWidth + input.Position.xyz, 1), MatWorld);
    output.Position = mul(float4(output.Position.xyz, 1), MatViewProj);
    output.TexCoord = input.TexCoord;
	return output;
}