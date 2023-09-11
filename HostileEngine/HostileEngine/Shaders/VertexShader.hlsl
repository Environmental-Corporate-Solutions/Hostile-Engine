#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ),"\
              "CBV(b0),"\
              "DescriptorTable(SRV(t0)), StaticSampler(s0)"

cbuffer ConstBuffer : register(b0)
{
    matrix mat;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float4 worldPos : WORLDPOS;
    float4 color : COLOR;
};

[RootSignature(MyRS1)]
VSOut main( float4 pos : POSITION, float4 normal : NORMAL)
{
    VSOut output;
    output.pos = mul(mat, pos);
    output.color = normal;
    output.worldPos = pos;
	return output;
}