#include "DXResource.h"
#include "DXContext.h"

void DXResource::SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint32 size)
{
	m_heap_properties =
	{
		.Type = heap_type,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0,
	};

	m_resource_desc =
	{
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		.Width = size,
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
		.Flags = resource_flags,
	};

	m_resource_state = D3D12_RESOURCE_STATE_COMMON;
}

void DXResource::CreateResource(DXContext& dx_context, const std::string& name_resource)
{
	dx_context.GetDevice()->CreateCommittedResource(&m_heap_properties, D3D12_HEAP_FLAG_NONE, &m_resource_desc, m_resource_state, nullptr, IID_PPV_ARGS(&m_resource)) >> CHK;
	NAME_DX_OBJECT(m_resource, name_resource);
}
