#include "Common.hlsl"

RWTexture2D<float4> m_uav : register(u0);

// For any form of Texture2D, you need a descriptor table
[RootSignature("RootFlags(0), DescriptorTable(UAV(u0))")]
[numthreads(8, 8, 1)]
void main
(
	const uint3 inGroupThreadID : SV_GroupThreadID,
	const uint3 inGroupID : SV_GroupID,
	const uint3 inDispatchThreadID : SV_DispatchThreadID,
	const uint inGroupIndex : SV_GroupIndex
)
{
	float2 uv = inDispatchThreadID.xy / float2(1500.0f, 800);
	m_uav[inDispatchThreadID.xy] = float4(uv,0,1);
}
