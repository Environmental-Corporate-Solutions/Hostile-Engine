SamplerState mySampler : register(s0);
Texture2D myTexture : register(t0);

struct VSOut
{
    float4 pos : SV_POSITION;
    float4 worldPos : WORLDPOS;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

#define PI 3.14159265f
float4 main(VSOut _in) : SV_TARGET
{
	return myTexture.Sample(mySampler, _in.uv);
}