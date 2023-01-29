//--------------------------------------------------------------------------------------
// File: Tutorial02.fx
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 );
Texture2D txLightDiffuse : register( t1 );

SamplerState samLinear : register( s0 );

cbuffer cbCamera : register(b0)
{
    matrix View;
}

cbuffer cbChangeOnResize : register(b1)
{
    matrix Projection;
}

cbuffer cbChangesEveryFrame : register(b2)
{
    matrix World;
    matrix WorldViewProjection;
}

cbuffer cbLight : register(b3)
{
    float4 vLightColor;
    float4 vLightDir;
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( input.Pos, WorldViewProjection );
    output.Norm = mul(float4(input.Norm, 1), World).xyz;

    output.Tex = input.Tex;

    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
// ������ ���� �ȴ�. ������ �� �� �Լ����� �� �����صθ�. (���� ���̴� ������ �صΰ�, blob�� �ٲ㼭 ��Ÿ�ӿ� ������ �ϴ°�?)
//--------------------------------------------------------------------------------------
float4 PS_TextureAndLighting( PS_INPUT input) : SV_Target
{
    float4 finalColor = txDiffuse.Sample(samLinear, input.Tex);
    finalColor += saturate(dot(input.Norm, (float3)vLightDir) * vLightColor);
    finalColor.a = 1;
    return finalColor;
}

float4 PS_Lighting( PS_INPUT input) : SV_Target
{
    float4 finalColor = (float4)0;
    finalColor += saturate(dot(input.Norm, (float3)vLightDir) * vLightColor)-0.3f;
    finalColor.a = 1;
    return finalColor;
}

float4 PS_Default( PS_INPUT input) : SV_Target
{
    float4 finalColor = (float4)0;
    finalColor += saturate(dot(input.Norm, (float3)vLightDir) * vLightColor);
    finalColor.a = 1;
    return finalColor;
}