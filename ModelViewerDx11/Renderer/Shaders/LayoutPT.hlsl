struct VsInput
{
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(VsInput input  ) : SV_POSITION
{
	return input.Position;
}