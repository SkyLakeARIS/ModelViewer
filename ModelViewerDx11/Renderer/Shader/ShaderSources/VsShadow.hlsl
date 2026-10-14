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

cbuffer CbMatLightViewProj : register(b2)
{
    matrix MatLightViewProj;
}

struct VsInput
{
    [[vk::location(0)]] float4 Position : POSITION;
};

struct PsInput
{
    float4 Position : SV_POSITION;
};

PsInput main(VsInput vsInput)
{
    PsInput psInput;
    psInput.Position = mul(vsInput.Position, MatWorld);
    psInput.Position = mul(psInput.Position, MatLightViewProj);
    return psInput;
}
