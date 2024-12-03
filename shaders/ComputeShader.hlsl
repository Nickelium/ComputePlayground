#include "Common.hlsl"
// TODO shader assert readback
// TODO shader printf readback
//https://therealmjp.github.io/posts/hlsl-printf/
// TODO shader printf GPU
// TODO shader unit test readback
// TODO shader debug draw
//https://www.gijskaerts.com/wordpress/?p=190

struct MyCBuffer
{
	int2 resolution;
	float iTime;
	int bindless_index;
};

ConstantBuffer<MyCBuffer> m_cbuffer : register(b0);

//#define CHEAP_STAR

float cheap_star(float2 uv, float anim)
{
	uv = abs(uv);
	float2 pos = min(uv.xy/uv.yx, anim);
	float p = (2.0 - pos.x - pos.y);
	return (2.0+p*(p*p-1.5)) / (uv.x+uv.y);      
}

float3 animate_star(float2 uv, float iTime)
{
	uv += -0.5f;
	uv *= 2.0 * ( cos(iTime * 2.0) -2.5); // scale
	float anim = sin(iTime * 12.0) * 0.1 + 1.0;  // anim between 0.9 - 1.1 
	return float3(cheap_star(uv, anim) * float3(0.35,0.2,0.15));
}

[RootSignature(ROOTFLAGS_DEFAULT ", RootConstants(num32BitConstants=4, b0)")]
[numthreads(8, 8, 1)]
void main
(
	const uint3 inGroupThreadID : SV_GroupThreadID,
	const uint3 inGroupID : SV_GroupID,
	const uint3 inDispatchThreadID : SV_DispatchThreadID,
	const uint inGroupIndex : SV_GroupIndex
)
{
	float2 uv = (inDispatchThreadID.xy + 0.5f) / float2(m_cbuffer.resolution.x, m_cbuffer.resolution.y);
	RWTexture2D<float4> m_uav = ResourceDescriptorHeap[m_cbuffer.bindless_index];
	float3 out_color;
#if defined(CHEAP_STAR)
	out_color = animate_star(uv, m_cbuffer.iTime);
#else
	out_color = float3((sin(m_cbuffer.iTime * 1.5) + 1) * 0.25 + uv.x, (cos(m_cbuffer.iTime * 2) + 1) * 0.125 + uv.y, 0);
	out_color = float3(uv, 0.0f);
	out_color = sRGBToLinear(out_color);
#endif
	// Presentation to display
	m_uav[inDispatchThreadID.xy] = float4(LinearTosRGB(out_color), 1.0f);
}