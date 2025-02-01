#include "DXResource.h"
#include "DXContext.h"
#include "DXQuery.h"

// We dont support custom heap
// Same as ID3D12Device::GetCustomHeapProperties
D3D12_HEAP_PROPERTIES g_heapPropertiesNUMA[D3D12_HEAP_TYPE_CUSTOM - 1]
{
	//D3D12_HEAP_TYPE_DEFAULT
	{
		.Type = D3D12_HEAP_TYPE_CUSTOM,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
		// GPU local memory
		.MemoryPoolPreference = D3D12_MEMORY_POOL_L1,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0,
	},
	//D3D12_HEAP_TYPE_UPLOAD
	{
		.Type = D3D12_HEAP_TYPE_CUSTOM,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE,
		// System memory
		.MemoryPoolPreference = D3D12_MEMORY_POOL_L0,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0,
	},
	//D3D12_HEAP_TYPE_READBACK
	{
		.Type = D3D12_HEAP_TYPE_CUSTOM,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		// System memory
		.MemoryPoolPreference = D3D12_MEMORY_POOL_L0,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0,
	},
};

// Note that D3D12_HEAP_TYPE_GPU_UPLOAD doesnt work on warp
void DXResource::SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint64 bytes)
{
	m_size_in_bytes = bytes;

	ASSERT(heap_type != D3D12_HEAP_TYPE_CUSTOM);
	m_heap_properties = g_heapPropertiesNUMA[heap_type];

	m_resource_desc =
	{
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		.Width = m_size_in_bytes,
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

	// In general avoid common and generic state
	// Generic is OR'd of multiple states, implies penalties of each contained states
	// Common is only needed for copy queue by specs requirement for some hardware reasons
	m_resource_state = D3D12_RESOURCE_STATE_COMMON;
}

ResourceAllocator g_resource_allocator;

void DXResource::CreateResource(DXContext& dx_context, const std::string& name_resource)
{
//	ComPtr<ID3D12Resource> resource = g_resource_allocator.CreateResource
//	(
//		dx_context, m_heap_properties, m_resource_desc, m_resource_state
//	);
	auto[resource, heap] = g_resource_allocator.CreateResourceAndHeap
	(
		dx_context, m_heap_properties, m_heap_flags, m_resource_desc, m_resource_state
	);
	m_heap = heap;
	m_resource = resource;
	NAME_DX_OBJECT(m_resource, name_resource);
#if !defined(_DEBUG)
	UNUSED(name_resource);
#endif
}

void DXVertexBufferResource::SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint32 size, uint32 stride)
{
	DXResource::SetResourceInfo(heap_type, resource_flags, size);
	// Less than 4GB
	ASSERT(m_size_in_bytes < UINT32_MAX);
	m_stride = stride;
	m_count = (uint32)m_size_in_bytes / m_stride;
}

void DXVertexBufferResource::CreateResource(DXContext& dx_context, const std::string& name_resource)
{
	// Less than 4GB
	ASSERT(m_size_in_bytes < UINT32_MAX);
	DXResource::CreateResource(dx_context, name_resource);
	m_vertex_buffer_view =
	{
		.BufferLocation = m_resource->GetGPUVirtualAddress(),
		.SizeInBytes = (uint32)m_size_in_bytes,
		.StrideInBytes = m_stride,
	};
}

void DXTextureResource::SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_HEAP_FLAGS heap_flags, D3D12_RESOURCE_FLAGS resource_flags, uint32 width, uint32 height, DXGI_FORMAT format)
{
	m_heap_flags = heap_flags;

	m_width = width;
	m_height = height;
	m_size_in_bytes = m_width * m_height;
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

ComPtr<ID3D12Heap> CreateHeap
 (
	 DXContext& dx_context, 
	 const D3D12_HEAP_PROPERTIES& heap_properties, 
	 uint64 bytes,
	 D3D12_HEAP_FLAGS heap_flags
 )
{
	ComPtr<ID3D12Heap> heap{};
	D3D12_HEAP_DESC heap_desc
	{
		.SizeInBytes = bytes,
		.Properties = heap_properties,
		// We dont support D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT
		.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		// Unfortunately 1080 TI only support Resource Heap Tier 1, which means heap types cant be mixed together
		.Flags = heap_flags,
	};
	dx_context.GetDevice()->CreateHeap(&heap_desc, IID_PPV_ARGS(&heap)) >> CHK;
	return heap;
}
ComPtr<ID3D12Resource> CreateResourceFromHeap
(
	DXContext& dx_context,
	::ComPtr<ID3D12Heap> heap,
	uint64 heap_offset,
	D3D12_RESOURCE_DESC resource_desc,
	D3D12_RESOURCE_STATES resource_state
)
{
	ComPtr<ID3D12Resource> resource{};
	dx_context.GetDevice()->CreatePlacedResource(heap.Get(), heap_offset, &resource_desc, resource_state, nullptr, IID_PPV_ARGS(&resource)) >> CHK;
	return resource;
}

std::pair
<
	ComPtr<ID3D12Resource>, 
	ComPtr<ID3D12Heap>
>
CreateResourceAndHeap
(	
	DXContext& dx_context, 
    D3D12_HEAP_PROPERTIES heap_properties,
	D3D12_HEAP_FLAGS heap_flags,
    D3D12_RESOURCE_DESC resource_desc,
    D3D12_RESOURCE_STATES resource_state
)
{
	D3D12_RESOURCE_ALLOCATION_INFO resource_allocation_info = dx_context.GetDevice()->GetResourceAllocationInfo(0, 1, &resource_desc);
	uint64 heap_size = resource_allocation_info.SizeInBytes + resource_allocation_info.Alignment;
	ComPtr<ID3D12Heap> heap = CreateHeap(dx_context, heap_properties, heap_size, heap_flags);
	uint64 heap_offset = resource_allocation_info.Alignment;
	ComPtr<ID3D12Resource> resource = CreateResourceFromHeap(dx_context, heap, heap_offset, resource_desc, resource_state);
	return { resource, heap };
}

std::pair
<
	ComPtr<ID3D12Resource>, 
	ComPtr<ID3D12Heap>
>
ResourceAllocator::CreateResourceAndHeap
(	
	DXContext& dx_context, 
	D3D12_HEAP_PROPERTIES heap_properties,
	D3D12_HEAP_FLAGS heap_flags,
	D3D12_RESOURCE_DESC resource_desc,
	D3D12_RESOURCE_STATES resource_state
)
{
	return ::CreateResourceAndHeap(dx_context, heap_properties, heap_flags, resource_desc, resource_state);
}

ComPtr<ID3D12Resource> ResourceAllocator::CreateResource
(
	DXContext& dx_context, 
	D3D12_HEAP_PROPERTIES heap_properties,
	D3D12_RESOURCE_DESC resource_desc,
	D3D12_RESOURCE_STATES resource_state
) 
{
	ComPtr<ID3D12Resource> final_resource{};
	if (false)
	//if (HasCachedResource( { resource_desc, heap_properties }))
	{
		auto [resource, old_resource_state] = GetCachedResource( { resource_desc, heap_properties });
		final_resource = resource;
		// Barrier
		// if different state transition
		if (old_resource_state != resource_state)
		{
			//ValidateResourceTransition(m_queue_graphics, m_command_list_graphics, resource);
			D3D12_RESOURCE_BARRIER barrier[1]{};
			barrier[0] =
			{
				.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
				.Transition =
				{
					.pResource = resource.Get(),
					.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					.StateBefore = old_resource_state,
					.StateAfter = resource_state,
				}
			};
			dx_context.GetCommandListGraphics()->ResourceBarrier(COUNT(barrier), barrier);
			//ValidateResourceTransition(m_queue_graphics, m_command_list_graphics, resource);
		}

	}
	else
	{
		dx_context.GetDevice()->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, resource_state, nullptr, IID_PPV_ARGS(&final_resource)) >> CHK;
	}
	return final_resource;
}

bool ResourceAllocator::HasCachedResource(const ResourceDescription& resource_description) const
{
	// TODO hashing
	for (uint32 i = 0; i < m_free_resource_list.size(); ++i)
	{
		auto [current_resource_description, current_resource] = m_free_resource_list[i];
		if (current_resource_description == resource_description)
		{
			return true;
		}
	}
	return false;
}

std::pair<ComPtr<ID3D12Resource>, D3D12_RESOURCE_STATES> ResourceAllocator::GetCachedResource(const ResourceDescription& resource_description)
{
	for (auto it = m_free_resource_list.begin(); it != m_free_resource_list.end(); ++it)
	{
		auto [current_resource_description, current_resource] = *it;
		if (current_resource_description == resource_description)
		{
			m_free_resource_list.erase(it);
			return current_resource;
		}
	}
	ASSERT(false);
	return {};
}

void ResourceAllocator::AddCachedResource(const ResourceDescription& resource_description, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES resource_state)
{
	std::pair<ResourceDescription, ComPtr < ID3D12Resource> > new_element { resource_description, resource };
	if (std::find_if
	(
		m_free_resource_list.begin(), m_free_resource_list.end(), 
		[resource_description, resource](std::pair<ResourceDescription, std::pair<ComPtr<ID3D12Resource>, D3D12_RESOURCE_STATES>> element)
		{
			return element.first == resource_description && element.second.first == resource;
		}
	) == m_free_resource_list.end())
	m_free_resource_list.push_back( { resource_description, { resource, resource_state } });
}

void ResourceHandler::RegisterResource(DXResource& resource)
{
	m_resources[g_current_buffer_index].push_back(resource);
}

void ResourceHandler::ReRegisterResource(DXResource& resource)
{
	auto it = std::find_if(m_resources[g_current_buffer_index].begin(), m_resources[g_current_buffer_index].end(), 
	[resource](DXResource element)
	{
		return element.m_resource == resource.m_resource;
	});
	if (it != m_resources[g_current_buffer_index].end())
	{
		// Patch up
		(*it).m_resource_state = resource.m_resource_state;
	}
	//m_resources[g_current_buffer_index].push_back(resource);
}

void ResourceHandler::FreeResources()
{
//	for (uint32 i = 0; i < m_resources[g_current_buffer_index].size(); ++i)
//	{
//		const DXResource& resource = m_resources[g_current_buffer_index][i];
//		g_resource_allocator.AddCachedResource( { resource.m_resource_desc, resource.m_heap_properties }, resource.m_resource, resource.m_resource_state);
//	}
	m_resources[g_current_buffer_index].clear();
}