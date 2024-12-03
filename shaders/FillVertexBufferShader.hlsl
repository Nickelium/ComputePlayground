#include "Common.hlsl"
// TODO shader debug draw
//https://www.gijskaerts.com/wordpress/?p=190

struct Vertex
{
	float2 position;
	float3 color;
};

struct MyCBuffer
{
	int bindless_index;
};

ConstantBuffer<MyCBuffer> m_cbuffer : register(b0);

[RootSignature(ROOTFLAGS_DEFAULT ", RootConstants(num32BitConstants=1, b0)")]
[numthreads(1, 1, 1)]
void main()
{
	RWStructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[m_cbuffer.bindless_index];
	Vertex vertex0;
	vertex0.position = float2(0.0f, +0.25f);
	vertex0.color = float3(1.0f, 0.0f, 0.0f);
	vertex_buffer[0] = vertex0;

	Vertex vertex1;
	vertex1.position = float2(+0.25f, -0.25f);
	vertex1.color = float3(0.0f, 1.0f, 0.0f);
	vertex_buffer[1] = vertex1;

	Vertex vertex2;
	vertex2.position = float2(-0.25f, -0.25f);
	vertex2.color = float3(0.0f, 0.0f, 1.0f);
	vertex_buffer[2] = vertex2;
}