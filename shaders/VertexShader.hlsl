#include "Common.hlsl"

struct VSInput
{
	float2 pos : Position;
	float3 color : Color;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float3 color : COLOR;
};

struct MyCBuffer
{
	int bindless_index;
};

ConstantBuffer<MyCBuffer> m_cbuffer : register(b0);

[RootSignature("RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), RootConstants(num32BitConstants=1, b0)")]
VSOutput main(uint vertex_id : SV_VERTEXID, uint instance_id : SV_InstanceID) 
{
	StructuredBuffer<VSInput> vertex_buffer = ResourceDescriptorHeap[m_cbuffer.bindless_index];
	VSInput input = vertex_buffer[vertex_id];

	VSOutput output = (VSOutput)0;
	output.pos = float4(input.pos, 0, 1);
	output.pos.xy /= 6;
	output.pos += float4(-1 + 0.25f + (instance_id % 4) * 0.5f, -0.75f + (instance_id / 4) * 0.5f, 0, 0);
	
    output.color = input.color;
	return output;
}