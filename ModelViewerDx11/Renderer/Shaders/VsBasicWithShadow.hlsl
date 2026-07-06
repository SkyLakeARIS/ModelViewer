
cbuffer cbMatrices : register(b0)
{
    matrix MatWorld;
}

cbuffer cbLightMatrix : register(b1)
{
    matrix MatLightViewProj;
}

cbuffer cbLightProperty : register(b2)
{
    float4 vLightColor;
    float4 vLightDir;
}

cbuffer CbCamera : register(b3)
{
    float3 Position;
    float Reserve;
}

cbuffer cbViewProj : register(b4)
{
    matrix MatViewProj;
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
    float3 Norm : NORMAL;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
    float3 LightColor : TEXCOORD1;
    float3 LightDir : TEXCOORD2;
    float3 CameraDir : TEXCOORD3;
    float4 ClipPosition : TEXCOORD4;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    matrix matWVP = MatWorld * MatViewProj;

    //float3 worldPosition = mul(input.Pos, World).xyz;
    float3 worldPosition = mul(input.Pos, MatWorld).xyz;

   // output.Pos = mul(input.Pos, WVP);
    output.Pos = mul(input.Pos, MatWorld);
    output.Pos = mul(output.Pos, MatViewProj);
    output.ClipPosition = mul(input.Pos, MatWorld);
    output.ClipPosition = mul(output.ClipPosition, MatLightViewProj);

    output.LightDir = normalize(worldPosition - vLightDir);

    output.UV = input.Tex;
    output.LightColor = vLightColor.xyz;
    output.CameraDir = normalize(worldPosition - Position);

    return output;
}
