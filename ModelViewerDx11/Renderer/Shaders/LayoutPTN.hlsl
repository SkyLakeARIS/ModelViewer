struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
    float3 Norm : NORMAL;
};


float4 main(VS_INPUT input ) : SV_POSITION
{
	return input.Pos;
}