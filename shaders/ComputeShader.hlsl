#include "Common.hlsl"

//RWTexture2D<float4> m_uav : register(u0);

struct MyCBuffer
{
	int2 resolution;
	int bindless_index;
};
ConstantBuffer<MyCBuffer> m_cbuffer : register(b0);
// TODO bindless
// For any form of Texture2D, you need a descriptor table
[RootSignature("RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), RootConstants(num32BitConstants=3, b0)")]
[numthreads(8, 8, 1)]
void main
(
	const uint3 inGroupThreadID : SV_GroupThreadID,
	const uint3 inGroupID : SV_GroupID,
	const uint3 inDispatchThreadID : SV_DispatchThreadID,
	const uint inGroupIndex : SV_GroupIndex
)
{
	float2 uv = inDispatchThreadID.xy / float2(m_cbuffer.resolution.x, m_cbuffer.resolution.y);
	RWTexture2D<float4> m_uav = ResourceDescriptorHeap[m_cbuffer.bindless_index];
	m_uav[inDispatchThreadID.xy] = float4(uv,0,1);
}
