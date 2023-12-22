RWStructuredBuffer<float4> m_uav : register(u0, space0);

[RootSignature("RootFlags(0), UAV(u0)")]
[numthreads(1024, 1, 1)]
void main
(
	uint3 inGroupThreadID : SV_GroupThreadID,
	uint3 inGroupID : SV_GroupID,
	uint3 inDispatchThreadID : SV_DispatchThreadID,
	uint inGroupIndex : SV_GroupIndex
)
{
	m_uav[inDispatchThreadID.x] = 0;	
	m_uav[inDispatchThreadID.x] = WaveGetLaneIndex();

	//ComplexNumber c = ComplexNumber(1, 1) + ComplexNumber(1, 2);
	//uav[inDispatchThreadID.x] = c.Imaginary;
}
