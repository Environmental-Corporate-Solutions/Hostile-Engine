#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ),"\
              "CBV(b0, visibility=SHADER_VISIBILITY_ALL)," \
              "CBV(b1, visibility=SHADER_VISIBILITY_ALL),"\
              "CBV(b2, visibility=SHADER_VISIBILITY_ALL),"  \
              "DescriptorTable(SRV(t0)), StaticSampler(s0)"

#define SkyboxRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ),"\
                 "CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
                 "DescriptorTable(SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),"\
                 "StaticSampler(s0,filter=FILTER_MIN_MAG_MIP_POINT , addressU=TEXTURE_ADDRESS_WRAP, addressV=TEXTURE_ADDRESS_WRAP, addressW=TEXTURE_ADDRESS_CLAMP)"

#define PI 3.14159265f
#define F3(x) float3(x, x, x)
        

struct Constants
{
    matrix viewProjection;
    float3 cameraPosition;
    float4 ambient_light;
};

struct Object
{
    matrix world;
    matrix normalWorld;
    uint id;
};

struct Material
{
    float3 albedo;             
    float roughness; 
    float emissive;
    float metalness;
};

ConstantBuffer<Constants> g_constants   : register(b0);        
ConstantBuffer<Material>  g_material    : register(b1);
ConstantBuffer<Object>    g_object      : register(b2);

struct VSIn
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct VSOut
{
    float4 pos      : SV_POSITION;
    float3 worldPos : WORLDPOS;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct PSOut
{
    float4 color : SV_Target0;
    float4 world_pos : SV_Target1;
    float4 normal : SV_Target2;
};

[RootSignature(MyRS1)]
VSOut main(VSIn _input)
{
    VSOut output;
    
    output.pos      = mul(g_constants.viewProjection, mul(g_object.world, float4(_input.position, 1)));
    output.worldPos = mul(g_object.world, float4(_input.position, 1)).xyz;
    output.normal   = mul((float3x3)g_object.normalWorld, _input.normal).xyz;
    output.texCoord = _input.texCoord;
    
    return output;
}

[RootSignature(SkyboxRS)]
VSOut VSSkyboxMain(VSIn _input)
{
    VSOut output;

    float3 outpos = _input.position + g_constants.cameraPosition;

    output.pos      = mul(g_constants.viewProjection, float4(outpos, 1)).xyww;
    output.worldPos = float4(_input.position, 1);
    output.texCoord = _input.texCoord;

    return output;
}

SamplerState g_skyboxSampler : register(s0);
Texture2D g_skyboxTexture : register(t0);

PSOut PSSkyboxMain(VSOut _input) 
{
    float3 p = normalize(_input.worldPos);
    float r = sqrt((p.x * p.x) + (p.y * p.y) + (p.z * p.z));
    float latitude = acos(p.y / r);
    float longitude = atan2(p.z, p.x);
    float2 coords = float2(longitude / (2 * PI), (latitude) / PI); //float2(longitude, latitude) * float2(0.5f / PI, 1.0f / PI);
    float2 UV = coords;
   
    PSOut ps_out;
    ps_out.color = g_skyboxTexture.Sample(g_skyboxSampler, UV);
    ps_out.normal = float4(0,0,0,0);
    ps_out.world_pos = float4(0,0,0,0);
    return ps_out;
}

float2 OctWrap(float2 V)
{
    return (1.0f - abs(V.yx)) * (V.xy >= 0.0f ? 1.0f : -1.0f);
}

float2 EncodeNormal(float3 N)
{
    N /= (abs(N.x) + abs(N.y) + abs(N.z));
    N.xy = N.z >= 0.0f ? N.xy : OctWrap(N.xy);
    N.xy = N.xy * 0.5f + 0.5f;
    return N.xy;
}

PSOut PSmain(VSOut _input)
{
    
    PSOut output;
    output.color = float4(g_material.albedo, g_material.roughness);
    output.normal.w = (float)g_object.id;
    output.normal.xy = EncodeNormal(_input.normal);
    output.normal.z = g_material.metalness;
    output.world_pos = float4(_input.worldPos, g_material.emissive);
    return output;
}