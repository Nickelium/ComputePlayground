#include "Common.hlsl"
// TODO shader debug draw
//https://www.gijskaerts.com/wordpress/?p=190

struct D3D12_DRAW_ARGUMENTS
{
	uint VertexCountPerInstance;
	uint InstanceCount;
	uint StartVertexLocation;
	uint StartInstanceLocation;
};

struct MyCBuffer
{
	int bindless_index;
};

ConstantBuffer<MyCBuffer> m_cbuffer : register(b0);

[RootSignature("RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), RootConstants(num32BitConstants=1, b0)")]
[numthreads(1, 1, 1)]
void main()
{
	RWStructuredBuffer<D3D12_DRAW_ARGUMENTS> OutDrawArgs = ResourceDescriptorHeap[m_cbuffer.bindless_index];

	D3D12_DRAW_ARGUMENTS args;
	args.VertexCountPerInstance = 3;
	args.InstanceCount = 16;
	args.StartVertexLocation = 0;
	args.StartInstanceLocation = 0;
	OutDrawArgs[0] = args;
}