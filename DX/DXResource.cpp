#include "DXResource.h"
#include "DXContext.h"
#include "DXQuery.h"

void DXResource::SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint32 size)
{
	m_size = size;

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
		.Width = m_size,
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

void DXVertexBufferResource::SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint32 size, uint32 stride)
{
	DXResource::SetResourceInfo(heap_type, resource_flags, size);
	m_stride = stride;
	m_count = m_size / m_stride;
}

void DXVertexBufferResource::CreateResource(DXContext& dx_context, const std::string& name_resource)
{
	DXResource::CreateResource(dx_context, name_resource);
	m_vertex_buffer_view =
	{
		.BufferLocation = m_resource->GetGPUVirtualAddress(),
		.SizeInBytes = m_size,
		.StrideInBytes = m_stride,
	};
}

void DXTextureResource::SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint32 width, uint32 height, DXGI_FORMAT format)
{
	m_width = width;
	m_height = height;
	m_size = m_width * m_height;
	m_format = format;

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
		.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		.Width = m_width,
		.Height = m_height,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = m_format,
		.SampleDesc =
		{
			.Count = 1,
			.Quality = 0,
		},
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = resource_flags,
	};

	m_resource_state = D3D12_RESOURCE_STATE_COMMON;
}

void DXTextureResource::CreateResource(DXContext& dx_context, const std::string& name_resource)
{
	DXResource::CreateResource(dx_context, name_resource);
}