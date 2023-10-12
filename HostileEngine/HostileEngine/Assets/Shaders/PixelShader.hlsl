SamplerState mySampler : register(s0);
Texture2D myTexture : register(t0);
#define PI 3.14159265f
#define F3(x) float3(x, x, x)

struct Light
{
    float3 lightPosition;
    float3 lightColor;
    bool active;
};

struct Constants
{
    matrix viewProjection;
    float3 cameraPosition;

    Light lights[16];
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
};



ConstantBuffer<Constants> g_constants   : register(b0);
ConstantBuffer<Material>  g_material    : register(b1);
ConstantBuffer<Object>    g_object      : register(b2);

struct VSOut
{
    float4 pos      : SV_POSITION;
    float3 worldPos : WORLDPOS;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
};

float DistributionGGX(float _nDotH, float _roughness)
{
    float a = _roughness * _roughness;
    float a2 = a * a;
    float denom = (_nDotH * _nDotH) * (a2 - 1.0f) + 1.0f;
    denom = PI * (denom * denom);
    return (a2 / denom);
}

float GeometrySmith(float _nDotV, float _nDotL, float _roughness)
{
    float r = _roughness + 1.0f;
    float k = (r * r) / 8.0f;
    float ggx1 = _nDotV / (_nDotV * (1.0f - k) + k);
    float ggx2 = _nDotL / (_nDotL * (1.0f - k) + k);
    return ggx1 * ggx2;
}

float3 FresnelSchlick(float _hDotV, float3 _baseReflectivity)
{
    float power = (-5.55473f * _hDotV - 6.98316) * _hDotV;
    return _baseReflectivity + (1.0f - _baseReflectivity) * pow(2, power);
}

float4 main(VSOut _in) : SV_TARGET
{
    float3 N = normalize(_in.normal);
    float3 V = normalize(g_constants.cameraPosition - _in.worldPos);

    float3 baseReflectivity = F3(0.04f);

    baseReflectivity = lerp(F3(0.04f), g_material.albedo, g_material.metalness);

    float3 lightOutput = F3(0);
    for (int i = 0; i < 16; i++)
    {
        if (!g_constants.lights[i].active)
            continue;
        
        float3 L = normalize(g_constants.lights[i].lightPosition);
        float3 H = normalize(V + L);

        float distance    = length(g_constants.lights[i].lightPosition - _in.worldPos);
        float attenuation = 1.0f / (distance * distance);
        float3 radiance   = g_constants.lights[i].lightColor * attenuation;

        float nDotV = max(dot(N, V), 0.0000001f);
        float nDotL = max(dot(N, L), 0.0000001f);
        float hDotV = max(dot(H, V), 0.0f);
        float nDotH = max(dot(N, H), 0.0f);

        float  D = DistributionGGX(nDotH, g_material.roughness);
        float  G = GeometrySmith(nDotV, nDotL, g_material.roughness);
        float3 F = FresnelSchlick(hDotV, baseReflectivity);

        float3 specular = D * G * F;
        specular /= (4.0f * nDotV * nDotL);

        float3 kd = F3(1.0f) - F;
        kd *= 1.0f - g_material.metalness;

        lightOutput += (kd * g_material.albedo / PI + specular) * radiance * nDotL;
    }

    float3 ambient = F3(0.03) * g_material.albedo;
    
    float3 color = ambient + lightOutput;

    color = color / (color + F3(1.0f));
    color = pow(color, F3(1.0f/2.2f));

    return float4(color, 1);
}