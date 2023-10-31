#define OutlineRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ),"\
                 "DescriptorTable(SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),"\
                 "StaticSampler(s0,filter=FILTER_MIN_MAG_MIP_POINT , addressU=TEXTURE_ADDRESS_WRAP, addressV=TEXTURE_ADDRESS_WRAP, addressW=TEXTURE_ADDRESS_CLAMP)"

struct VSIn
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct VSOut
{
    float4 pos      : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D<uint> g_texture : register(t0);
SamplerState g_sampler : register(s0);

[RootSignature(OutlineRS)]
VSOut main(VSIn _in)
{
    VSOut output;
    output.pos = float4(_in.position, 1);
    output.texCoord = _in.texCoord;
    
    return output;
}

float4 PSmain(VSOut _in) : SV_Target
{
    uint p00 = g_texture.Load(int3(_in.texCoord.x * 1920, _in.texCoord.y * 1080, 0));
    uint p01 = g_texture.Load(int3((_in.texCoord.x + 1) * 1920, _in.texCoord.y * 1080, 0));
    uint p10 = g_texture.Load(int3(_in.texCoord.x * 1920, (_in.texCoord.y + 1) * 1080, 0));
    uint p11 = g_texture.Load(int3((_in.texCoord.x + 1) * 1920, (_in.texCoord.y+ 1) * 1080, 0));
    
    uint p = (p00 + p01 + p10 + p11) / 4;
    if (p != 1 && p != 0)
        return float4(1, 0, 0, 1);
    
    clip(-1);
    return float4(0, 0, 0, 0);
}
