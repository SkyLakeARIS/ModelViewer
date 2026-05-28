cbuffer CbWorld : register(b0)
{
    matrix MatWorld;
}

cbuffer CbViewProj : register(b1)
{
    matrix MatViewProj;
}

struct VsInput
{
    float4 Pos : POSITION;
};

struct PsInput
{
    float4 Pos : SV_POSITION;
};

PsInput main(VsInput input)
{
    PsInput output;
    output.Pos = mul(input.Pos, MatWorld);
    output.Pos = mul(output.Pos, MatViewProj);
    return output;
}