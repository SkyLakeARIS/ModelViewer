cbuffer cbColor : register(b0)
{
    float3 Color;
    float Reserve;
}


struct PsInput
{
    float4 Pos : SV_POSITION;
};


float4 main(PsInput input) : SV_TARGET
{
    return float4(Color, 1.0);
}