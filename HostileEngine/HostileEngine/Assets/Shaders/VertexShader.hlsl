#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ),"\
              "CBV(b0, visibility=SHADER_VISIBILITY_ALL)," \
              "CBV(b1, visibility=SHADER_VISIBILITY_ALL),"\
              "CBV(b2, visibility=SHADER_VISIBILITY_ALL),"  \
              "CBV(b3, visibility=SHADER_VISIBILITY_ALL),"  \
              "DescriptorTable(SRV(t0)), StaticSampler(s0)"

#define SkyboxRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ),"\
                 "CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
                 "DescriptorTable(SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),"\
                 "StaticSampler(s0,filter=FILTER_MIN_MAG_MIP_POINT , addressU=TEXTURE_ADDRESS_WRAP, addressV=TEXTURE_ADDRESS_WRAP, addressW=TEXTURE_ADDRESS_CLAMP)"

#define PI 3.14159265f
#define F3(x) float3(x, x, x)

struct Light
{
    float3 lightPosition;
    float4 lightColor;
};        

struct Constants
{
    matrix viewProjection;
    float3 cameraPosition;
};

struct Object
{
    matrix world;
    matrix normalWorld;
};

struct Material
{
    float3 albedo;
    float metalness; // F0
    float roughness; 
    float emissive;
};

cbuffer Lights : register(b1)
{
    Light g_lights[16];    
}
ConstantBuffer<Constants> g_constants   : register(b0);        
ConstantBuffer<Material>  g_material    : register(b2);
ConstantBuffer<Object>    g_object      : register(b3);

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

float4 PSSkyboxMain(VSOut _input) : SV_TARGET
{
    float3 p = normalize(_input.worldPos);
    float r = sqrt((p.x * p.x) + (p.y * p.y) + (p.z * p.z));
    float latitude = acos(p.y / r);
    float longitude = atan2(p.z, p.x);
    float2 coords = float2(longitude / (2 * PI), (latitude) / PI); //float2(longitude, latitude) * float2(0.5f / PI, 1.0f / PI);
    float2 UV = coords;
   
    return g_skyboxTexture.Sample(g_skyboxSampler, UV);
}

float NormalDistribution(float _nDotH)
{
    float alpha2 = g_material.roughness * g_material.roughness * g_material.roughness * g_material.roughness;
    float d = _nDotH * _nDotH * (alpha2 - 1) + 1;
    float distribution = alpha2 / (PI * d * d);
    return distribution;
}

float Geometry(float _nDotV)
{
    float k = ((g_material.roughness + 1) * (g_material.roughness + 1)) / 8.0f;
    float denom = _nDotV * (1 - k) + k;
    return _nDotV / denom;
}

float3 Fresnel(float _vDotH)
{
    float3 F0 = F3(0.04f);
    if (g_material.metalness)
        F0 = g_material.metalness * g_material.albedo;

    float3 value = F0 + (1 - F0) * pow(saturate(1.0 - _vDotH), 5);
    return value;
}

float4 PSmain(VSOut _input) : SV_TARGET
{
    float3 N = normalize(_input.normal);
    float3 V = normalize(g_constants.cameraPosition - _input.worldPos);

    float3 lightsOutput = F3(0.0f);
    for (int i = 0; i < 16; i++)
    {
        if (!g_lights[i].lightColor.w)
            continue;
        
        float3 L = normalize(g_lights[i].lightPosition - _input.worldPos);
        float3 H = normalize(V + L);

        
        float nDotL = saturate(dot(N, L));
        float nDotH = saturate(dot(N, H));
        float vDotH = saturate(dot(V, H));
        float nDotV = saturate(dot(N, V));

        float D = NormalDistribution(nDotH);
        float G = Geometry(nDotV) * Geometry(nDotL);
        float F = Fresnel(vDotH);

        float kS = F;
        float kD = 1.0f - kS;

        float specularBRDF = ((D * G * F) / (4 * nDotL * nDotV + 0.00001f));
        float diffuseBRDF = (kD * g_material.albedo) / PI;

        lightsOutput += (diffuseBRDF + specularBRDF) * (float3)g_lights[i].lightColor * nDotL;
    }

    lightsOutput = lightsOutput / (lightsOutput + F3(1.0f));
    float3 finalLight = pow(lightsOutput, F3(1.0 / 2.2));

    float3 ambient = F3(0.03f) * g_material.albedo;
    float3 emissive = g_material.albedo * g_material.emissive;
    
    float3 color = ambient + finalLight + emissive;

    return float4(color, 1);
}