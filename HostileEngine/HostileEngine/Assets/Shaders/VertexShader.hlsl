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

};

[RootSignature(MyRS1)]
VSOut main( float4 pos : POSITION)
{
    VSOut output;
    output.pos = mul(mat, pos);
    
    output.worldPos = pos;
	return output;
}