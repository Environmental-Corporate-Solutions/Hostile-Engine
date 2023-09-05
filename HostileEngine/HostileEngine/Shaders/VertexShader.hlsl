#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT )"

[RootSignature(MyRS1)]
float4 main( float4 pos : POSITION, float4 normal : NORMAL) : SV_POSITION
{
	return pos;
}