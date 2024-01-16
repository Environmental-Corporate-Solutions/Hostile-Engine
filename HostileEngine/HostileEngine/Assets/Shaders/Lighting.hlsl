#define LightingRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ),"\
              "CBV(b0, visibility=SHADER_VISIBILITY_ALL)," \
              "DescriptorTable(SRV(t0)),"\
              "DescriptorTable(SRV(t1)),"\
              "DescriptorTable(SRV(t2)),"\
              "DescriptorTable(SRV(t3)),"\
              "StaticSampler(s0)"
#define PI 3.14159265f
#define RECIPROCAL_PI (1 / PI)
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
    float pad[58];
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

float3 FresnelSchlick(float cos_theta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cos_theta, 5.0f);
}

float D_GGX(float NoH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    float b = (NoH2 * (alpha2 - 1.0f) + 1.0f);

    return alpha2 * RECIPROCAL_PI / (b * b); 
}

float G1_GGX_Schlick(float NoV, float roughness)
{
    float alpha = roughness * roughness;
    float k = alpha / 2.0f;
    return max(NoV, 0.001f) / (NoV * (1.0f - k) + k);
}

float G_Smith(float NoV, float NoL, float roughness)
{
    return G1_GGX_Schlick(NoL, roughness) * G1_GGX_Schlick(NoV, roughness);
}

float3 BRDFMicrofacet(float3 L, float3 V, float3 N,
    float metallic, float roughness, float3 base_color, float reflectance)
{
    float3 H = normalize(V + L);

    float NoV = clamp(dot(N, V), 0.0f, 1.0f);
    float NoL = clamp(dot(N, L), 0.0f, 1.0f);
    float NoH = clamp(dot(N, H), 0.0f, 1.0f);
    float VoH = clamp(dot(V, H), 0.0f, 1.0f);

    // for dialectrics, the max reflectance value is between 0 - .16
    float3 F0 = F3(0.16f * (reflectance * reflectance));
    // if the object is metallic, use the base color as the reflective coefficient
    F0 = lerp(F0, base_color, metallic);

    float3 F = FresnelSchlick(VoH, F0);
    float D = D_GGX(NoH, roughness);
    float G = G_Smith(NoV, NoL, roughness);

    float3 spec = (F * D * G) / (4.0f * max(NoV, 0.001f) * max(NoL, 0.001f));

    float3 rhoD = base_color;
    rhoD *= (F3(1.0f) - F);
    rhoD *= (1.0f - metallic);

    float3 diff = rhoD * RECIPROCAL_PI;

    return diff + spec;
}

float3 Decode(float2 F)
{
    F = F * 2.0f - 1.0f;
    float3 N = float3(F.x, F.y, 1.0f - abs(F.x) - abs(F.y));
    float t = saturate(-N.z);
    N.xy += N.xy >= 0.0f ? -t : t;
    return normalize(N);
}

float3 RGBtoLin(float3 rgb)
{
    return pow(rgb, F3(2.2f));
}

float3 LinToRGB(float3 lin)
{
    return pow(lin, F3(1.0f / 2.2f));
}

PSOut PSMain(VSOut _output)
{
    PSOut output;
    float4 normal = g_normal_map.Sample(g_sampler, _output.tex_coord);
    
    float3 world_pos = g_world_pos_map.Sample(g_sampler, _output.tex_coord).xyz;
    float3 albedo    = g_color_map.Sample(g_sampler, _output.tex_coord).xyz;
    
    float emissive = g_world_pos_map.Sample(g_sampler, _output.tex_coord).w;
    float roughness = g_color_map.Sample(g_sampler, _output.tex_coord).w;
    float metalness = normal.z;
    
    float3 N = Decode(normal.xy);
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

    float3 radiance = emissive * RGBtoLin(albedo);

    float3 L = normalize(g_lights[_output.instance].lightPosition - world_pos);
    float irradiance = max(dot(L, N), 0.0f) * 1.0f;

    if (irradiance > 0.0f)
    {
        float3 brdf = BRDFMicrofacet(L, V, N, metalness, roughness, albedo, 1);
        radiance += brdf * irradiance * g_lights[_output.instance].lightColor;
    }

    output.id = normal.w;
    output.color = float4(LinToRGB(radiance), 1);
    return output;
}