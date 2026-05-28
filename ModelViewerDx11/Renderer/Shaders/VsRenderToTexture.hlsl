
cbuffer cbChangesEveryFrame : register(b0)
{
    matrix MatWorld;
}

cbuffer cbViewProj : register(b1)
{
    matrix MatViewProj;
}


struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};


struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};



PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    output.Pos = mul(input.Pos, MatWorld);
    output.Pos = mul(output.Pos, MatViewProj);

    output.Tex = input.Tex;
	return output;
}