SamplerState mySampler : register(s0);
Texture2D myTexture : register(t0);

struct VSOut
{
    float4 pos : SV_POSITION;
    float4 worldPos : WORLDPOS;
    float4 color : COLOR;
};

#define PI 3.14159265f
float4 main(VSOut _in) : SV_TARGET
{
    float4 p = _in.worldPos;
    float2 UV;
    float theta = atan2(p.z, p.x);
    float r = sqrt((p.x * p.x) + (p.y * p.y) + (p.z * p.z));
    float phi = atan(p.y / r);
    UV.x = theta / (2 * PI);
    UV.y = (PI - phi) / PI;
    
	return myTexture.Sample(mySampler, UV);
}