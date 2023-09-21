#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ),"\
              "CBV(b0),"\
              "DescriptorTable(SRV(t0)), "\
              "StaticSampler(s0, filter = FILTER_ANISOTROPIC, addressU = TEXTURE_ADDRESS_WRAP,"\
              "addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)"

cbuffer ConstBuffer : register(b0)
{
    matrix model;
    matrix normalMat;
    matrix cam;
    matrix bones[200];
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float4 worldPos : WORLDPOS;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

[RootSignature(MyRS1)]
VSOut main(
    float4 pos       : POSITION, 
    float2 uv        : TEXCOORD, 
    float4 normal    : NORMAL,
    uint4  boneID[2] : BONEID,
    float4  weight[2] : WEIGHT
)
{
    VSOut output;
    
    float3 posL = 0;
    float weights[8] = { weight[0].x, weight[0].y, weight[0].z, weight[0].w, weight[1].x, weight[1].y, weight[1].z, weight[1].w };
    uint boneIndices[8] = { boneID[0].x, boneID[0].y, boneID[0].z, boneID[0].w, boneID[1].x, boneID[1].y, boneID[1].z, boneID[1].w };
    for (uint i = 0; i < 8; i++)
    {
        posL += weights[i] * mul(bones[boneIndices[i]], float4(pos.xyz, 1)).xyz;
    }
    
    

    output.pos      = mul(float4(posL, 1), cam);
    output.normal   = mul(normalMat, normal);
    output.worldPos = mul(model,  pos);
    output.uv = uv;
	return output;
}