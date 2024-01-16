#define LightingRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ),"\
              "CBV(b0, visibility=SHADER_VISIBILITY_ALL)," \
              "DescriptorTable(SRV(t0)),"\
              "DescriptorTable(SRV(t1)),"\
              "DescriptorTable(SRV(t2)),"\
              "DescriptorTable(SRV(t3)),"\
              "StaticSampler(s0)"
#define PI 3.14159265f
#define F3(x) float3(x, x, x)
struct Constants
{
    matrix viewProjection;
    float3 cameraPosition;
    float4 ambient_light;
};

struct Light
{
    float3 lightPosition;
    float3 lightColor;
}; 

ConstantBuffer<Constants> g_constants : register(b0);
StructuredBuffer<Light> g_lights : register(t0);

struct VSIn
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
    uint g_instance : SV_INSTANCEID;
};

struct VSOut
{
    float4 position : SV_Position;
    float2 tex_coord : TEXCOORD;
    uint instance : InstanceID;
};

SamplerState g_sampler : register(s0);
Texture2D g_color_map : register(t1);
Texture2D g_world_pos_map : register(t2);
Texture2D g_normal_map : register(t3);

[RootSignature(LightingRS)]
VSOut VSMain(VSIn _input)
{
    VSOut output;
    output.position = float4(_input.position, 1);
    output.tex_coord = _input.texCoord;
    output.instance = _input.g_instance;
    return output;
}

struct PSOut
{
    float4 color : SV_Target0;
    float id : SV_Target1;
};

float NormalDistribution(float _nDotH, float _roughness)
{
    float alpha2 = _roughness * _roughness * _roughness * _roughness;
    float d = _nDotH * _nDotH * (alpha2 - 1) + 1;
    float distribution = alpha2 / (PI * d * d);
    return distribution;
}

float Geometry(float _nDotV, float _roughness)
{
    float k = ((_roughness + 1) * (_roughness + 1)) / 8.0f;
    float denom = _nDotV * (1 - k) + k;
    return _nDotV / denom;
}

float3 Fresnel(float _vDotH, float _metalness, float3 _albedo)
{
    float3 F0 = F3(0.04f);
    if (_metalness)
        F0 = _metalness * _albedo;

    float3 value = F0 + (1 - F0) * pow(saturate(1.0 - _vDotH), 5);
    return value;
}

PSOut PSMain(VSOut _output)
{
    PSOut output;
    float4 normal = g_normal_map.Sample(g_sampler, _output.tex_coord);
    float3 world_pos = g_world_pos_map.Sample(g_sampler, _output.tex_coord).xyz;
    float emissive = g_world_pos_map.Sample(g_sampler, _output.tex_coord).w;
    float roughness = g_color_map.Sample(g_sampler, _output.tex_coord).w;
    float3 albedo = g_color_map.Sample(g_sampler, _output.tex_coord).xyz;
    float3 N = normalize(normal.xyz);
    float3 V = normalize(g_constants.cameraPosition - world_pos);
    if (world_pos.x == 0 && world_pos.y == 0 && world_pos.z == 0 
    && normal.x == 0 && normal.y == 0 && normal.z == 0)
    {
        output.color = float4(albedo, 1);
        output.id = 0;
        return output;
    }
    
    if (_output.instance == 0)
    {
        output.color = float4(albedo, 1) * g_constants.ambient_light;
        output.id = normal.w;
        return output;   
    }

    float3 lightsOutput = F3(0.0f);
    
    float3 L = normalize(g_lights[_output.instance].lightPosition - world_pos);
    float3 H = normalize(V + L);
    
    float nDotL = saturate(dot(N, L));
    float nDotH = saturate(dot(N, H));
    float vDotH = saturate(dot(V, H));
    float nDotV = saturate(dot(N, V));
    float D = NormalDistribution(nDotH, roughness);
    float G = Geometry(nDotV, roughness) * Geometry(nDotL, roughness);
    float F = Fresnel(vDotH, 1 - roughness, albedo);
    float kS = F;
    float kD = 1.0f - kS;
    float3 specularBRDF = ((D * G * F) / (4 * nDotL * nDotV + 0.00001f)) * (float3)g_lights[_output.instance].lightColor;
    float3 diffuseBRDF = ((float3)g_lights[_output.instance].lightColor * nDotL) / PI;
    lightsOutput += ((diffuseBRDF * albedo) + specularBRDF);

    lightsOutput = lightsOutput / (lightsOutput + F3(1.0f));
    float3 finalLight = pow(lightsOutput, F3(1.0 / 2.2));

    float3 ambient = F3(0.03f) * albedo;
    float3 emissive_final = albedo * emissive;
    
    float3 color = ambient + finalLight + emissive_final;
    output.color = float4(color, 1);
    output.id = normal.w;
    return output;
}