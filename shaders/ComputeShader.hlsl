#include "Common.hlsl"

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

float4 animate_star(float2 uv, float iTime)
{
	uv += -0.5f;
	uv *= 2.0 * ( cos(iTime * 2.0) -2.5); // scale
	float anim = sin(iTime * 12.0) * 0.1 + 1.0;  // anim between 0.9 - 1.1 
	return float4(cheap_star(uv, anim) * float3(0.35,0.2,0.15), 1.0);
}

// For any form of Texture2D, you need a descriptor table
[RootSignature("RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), RootConstants(num32BitConstants=4, b0)")]
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
	float4 out_color;
#if defined(CHEAP_STAR)
	out_color = animate_star(uv, m_cbuffer.iTime);
#else
	out_color = float4(sin(m_cbuffer.iTime), cos(uv.y), 0, 1);
#endif
	m_uav[inDispatchThreadID.xy] = out_color;
}