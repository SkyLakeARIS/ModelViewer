/*
    b0 ~ b5 는 렌더러에서 예약 (reserved slots by renderer)

    b0 : World Matrix
    b1 : View + Projection Matrix (mainly Perspective)
    b2 : View + Projection Matrix of Light
    b3 : Light Attributes (for lighting)
    b4 : Camera Attribute (for lighting)
    b5 : Orthographic Matrix (for screen)
 */

cbuffer CbMatWorld : register(b0)
{
    matrix MatWorld;
}

cbuffer CbMatViewProj : register(b1)
{
    matrix MatViewProj;
}

struct VsInput
{
    [[vk::location(0)]] float4 Position : POSITION;
};

struct PsInput
{
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD0;
    float3 WorldPosition : TEXCOORD2;
};

PsInput main(VsInput vsInput)
{
    PsInput psInput;

    // 깊이 값이 가장 뒤어야 하기 때문에 따로 다시 지정하기 보다 1인 w값을 대신 쓰는 것. w = 1, using w instead of z
    //output.Position = mul(float4(input.Position.xyz, 1), MatWorld).xyww;
    psInput.Position = mul(vsInput.Position, MatWorld);
    psInput.WorldPosition = psInput.Position;
    psInput.Position = mul(float4(psInput.Position.xyz, 1.0f), MatViewProj).xyww;

    // tex값은 위치 값
    psInput.TexCoord = vsInput.Position;
    return psInput;
}
