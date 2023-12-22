#include "DXResource.h"

UAV::UAV()
{
	m_HeapProperties =
	{
		.Type = D3D12_HEAP_TYPE_DEFAULT,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0,
	};

	m_Desc =
	{
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		.Width = 0,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc =
		{
			.Count = 1,
			.Quality = 0,
		},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR, // In order
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
	};

	m_ReadbackHeapProperties = m_HeapProperties;
	m_ReadbackHeapProperties.Type = D3D12_HEAP_TYPE_READBACK;

	m_ReadbackDesc = m_Desc;
	m_ReadbackDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
}
